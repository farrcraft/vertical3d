/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/scene/Scene.h>

#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace v3d::moya::scene {

Scene::Scene(const Schema & schema) : schema_(schema) {
}

const Schema & Scene::schema() const noexcept {
    return schema_;
}

Scene::Slot & Scene::slot(const Handle & object) {
    if (object.index >= slots_.size()) {
        throw std::invalid_argument("a handle that names no object");
    }
    Slot & found = slots_[object.index];
    if (!found.live || found.generation != object.generation || found.kind != object.kind) {
        throw std::invalid_argument("a handle that names no object");
    }
    return found;
}

const Declaration & Scene::declaration(const Slot & slot, const std::string & parameter) const {
    const Kind & kind = schema_.kind(slot.kind);
    for (const Declaration & declared : kind.parameters) {
        if (declared.name == parameter) {
            return declared;
        }
    }
    for (const Declaration & declared : slot.primvars) {
        if (declared.name == parameter) {
            return declared;
        }
    }
    throw std::invalid_argument("a " + kind.name + " has no parameter named " + parameter);
}

void Scene::checkTimes(const Declaration & declaration, const Samples & samples) {
    if (!samples.times.empty()) {
        if (!declaration.motion) {
            throw std::invalid_argument("parameter " + declaration.name + " holds no samples over the shutter");
        }
        if (samples.times.size() != samples.values.size()) {
            throw std::invalid_argument("parameter " + declaration.name + " set with " + std::to_string(samples.times.size()) + " times and " +
                std::to_string(samples.values.size()) + " values");
        }
        for (std::size_t i = 1; i < samples.times.size(); ++i) {
            if (!(samples.times[i] > samples.times[i - 1])) {
                throw std::invalid_argument("parameter " + declaration.name + " set at times that do not rise");
            }
        }
    }
}

void Scene::checkHandles(const Declaration & declaration, const Value & value) const {
    const std::optional<std::uint32_t> target = schema_.find(declaration.target);
    for (const Handle & named : value.handleArray()) {
        if (!named.valid() || named.kind != target) {
            throw std::invalid_argument("parameter " + declaration.name + " names something that is not a " + declaration.target);
        }
    }
}

void Scene::check(const Declaration & declaration, const Samples & samples) const {
    if (samples.values.empty()) {
        throw std::invalid_argument("parameter " + declaration.name + " set with no samples");
    }
    checkTimes(declaration, samples);
    const std::size_t size = samples.values.front().size();
    for (const Value & value : samples.values) {
        if (value.type() != declaration.type) {
            throw std::invalid_argument("parameter " + declaration.name + " is a " + name(declaration.type) + " and was set to a " +
                name(value.type()));
        }
        if (!declaration.array && value.size() != 1) {
            throw std::invalid_argument("parameter " + declaration.name + " holds one element and was set to " + std::to_string(value.size()));
        }
        if (value.size() != size) {
            throw std::invalid_argument("parameter " + declaration.name + " set to samples of different sizes");
        }
        if (value.type() == Type::Handle) {
            checkHandles(declaration, value);
        }
    }
}

Handle Scene::create(const std::string & kind) {
    const std::optional<std::uint32_t> index = schema_.find(kind);
    if (!index) {
        throw std::invalid_argument("no kind of object is named " + kind);
    }

    const std::scoped_lock lock(mutex_);
    std::uint32_t at = 0;
    if (free_.empty()) {
        at = static_cast<std::uint32_t>(slots_.size());
        slots_.emplace_back();
    } else {
        at = free_.back();
        free_.pop_back();
    }
    Slot & made = slots_[at];
    made.kind = *index;
    made.generation++;
    made.live = true;
    made.primvars.clear();

    const Handle handle { .kind = *index, .index = at, .generation = made.generation };
    pending_.push_back(Edit { .op = Edit::Op::Create, .handle = handle });
    return handle;
}

void Scene::set(const Handle & object, const std::string & parameter, const Value & value) {
    const Samples samples { .times = {}, .values = { value } };
    const std::scoped_lock lock(mutex_);
    check(declaration(slot(object), parameter), samples);
    pending_.push_back(Edit { .op = Edit::Op::Set, .handle = object, .parameter = parameter, .samples = samples });
}

void Scene::set(const Handle & object, const std::string & parameter, const std::vector<float> & times, const std::vector<Value> & values) {
    const Samples samples { .times = times, .values = values };
    const std::scoped_lock lock(mutex_);
    const Declaration & declared = declaration(slot(object), parameter);
    if (times.empty()) {
        throw std::invalid_argument("parameter " + parameter + " set with no times");
    }
    check(declared, samples);
    pending_.push_back(Edit { .op = Edit::Op::Set, .handle = object, .parameter = parameter, .samples = samples });
}

void Scene::declare(const Handle & object, const Declaration & primvar) {
    const std::scoped_lock lock(mutex_);
    Slot & found = slot(object);
    const Kind & kind = schema_.kind(found.kind);
    if (!kind.primvars) {
        throw std::invalid_argument("a " + kind.name + " holds no primvars");
    }
    for (const Declaration & declared : kind.parameters) {
        if (declared.name == primvar.name) {
            throw std::invalid_argument("primvar " + primvar.name + " is a parameter of a " + kind.name);
        }
    }
    schema_.check(primvar);

    const auto same = std::find_if(found.primvars.begin(), found.primvars.end(),
        [&primvar](const Declaration & declared) { return declared.name == primvar.name; });
    if (same == found.primvars.end()) {
        found.primvars.push_back(primvar);
    } else {
        *same = primvar;
    }
    pending_.push_back(Edit { .op = Edit::Op::Declare, .handle = object, .parameter = primvar.name, .primvar = primvar });
}

void Scene::remove(const Handle & object) {
    const std::scoped_lock lock(mutex_);
    Slot & found = slot(object);
    found.live = false;
    found.primvars.clear();
    free_.push_back(object.index);
    pending_.push_back(Edit { .op = Edit::Op::Remove, .handle = object });
}

void Scene::commit() {
    const std::scoped_lock lock(mutex_);
    if (!pending_.empty()) {
        committed_.push_back(std::move(pending_));
        pending_.clear();
    }
}

std::uint64_t Scene::apply() {
    std::vector<std::vector<Edit>> batches;
    {
        const std::scoped_lock lock(mutex_);
        batches.swap(committed_);
    }
    for (const std::vector<Edit> & batch : batches) {
        version_++;
        for (const Edit & edit : batch) {
            apply(edit, version_);
        }
    }
    return version_;
}

void Scene::apply(const Edit & edit, std::uint64_t version) {
    switch (edit.op) {
        case Edit::Op::Create: {
            if (objects_.size() <= edit.handle.index) {
                objects_.resize(static_cast<std::size_t>(edit.handle.index) + 1);
            }
            Object made { .handle = edit.handle, .created = version };
            for (const Declaration & declared : schema_.kind(edit.handle.kind).parameters) {
                made.parameters[declared.name] = Parameter { .samples = Samples { .times = {}, .values = { declared.fallback } }, .version = version };
            }
            objects_[edit.handle.index] = std::move(made);
            break;
        }
        case Edit::Op::Set:
            applied(edit.handle).parameters[edit.parameter] = Parameter { .samples = edit.samples, .version = version };
            break;
        case Edit::Op::Declare: {
            Object & object = applied(edit.handle);
            const auto same = std::find_if(object.primvars.begin(), object.primvars.end(),
                [&edit](const Declaration & declared) { return declared.name == edit.primvar.name; });
            if (same == object.primvars.end()) {
                object.primvars.push_back(edit.primvar);
            } else {
                *same = edit.primvar;
            }
            object.parameters[edit.primvar.name] = Parameter { .samples = Samples { .times = {}, .values = { edit.primvar.fallback } }, .version = version };
            break;
        }
        case Edit::Op::Remove:
            objects_[edit.handle.index].reset();
            removals_.emplace_back(version, edit.handle);
            break;
    }
}

std::uint64_t Scene::version() const noexcept {
    return version_;
}

Scene::Object & Scene::applied(const Handle & object) {
    std::optional<Object> & held = objects_.at(object.index);
    if (!held.has_value()) {
        throw std::logic_error("an edit of an object the scene does not hold");
    }
    return *held;
}

const Scene::Object * Scene::find(const Handle & object) const {
    if (object.index >= objects_.size()) {
        return nullptr;
    }
    const std::optional<Object> & held = objects_[object.index];
    if (!held.has_value() || held->handle != object) {
        return nullptr;
    }
    return &*held;
}

bool Scene::contains(const Handle & object) const {
    return find(object) != nullptr;
}

std::vector<Handle> Scene::objects(const std::string & kind) const {
    std::vector<Handle> found;
    const std::optional<std::uint32_t> index = schema_.find(kind);
    if (!index) {
        return found;
    }
    for (const std::optional<Object> & object : objects_) {
        if (object && object->handle.kind == *index) {
            found.push_back(object->handle);
        }
    }
    return found;
}

const Samples & Scene::samples(const Handle & object, const std::string & parameter) const {
    const Object * found = find(object);
    if (found == nullptr) {
        throw std::out_of_range("the scene holds no such object");
    }
    const auto at = found->parameters.find(parameter);
    if (at == found->parameters.end()) {
        throw std::out_of_range("the object has no parameter named " + parameter);
    }
    return at->second.samples;
}

const Value & Scene::value(const Handle & object, const std::string & parameter) const {
    return samples(object, parameter).values.front();
}

const std::vector<Declaration> & Scene::primvars(const Handle & object) const {
    const Object * found = find(object);
    if (found == nullptr) {
        throw std::out_of_range("the scene holds no such object");
    }
    return found->primvars;
}

std::vector<Change> Scene::changes(std::uint64_t since) const {
    std::vector<std::pair<std::uint32_t, Change>> found;
    for (const auto & [version, handle] : removals_) {
        if (version > since) {
            found.emplace_back(handle.index, Change { .handle = handle, .what = Change::What::Removed });
        }
    }
    for (const std::optional<Object> & held : objects_) {
        if (!held.has_value()) {
            continue;
        }
        const Object & object = *held;
        if (object.created > since) {
            found.emplace_back(object.handle.index, Change { .handle = object.handle, .what = Change::What::Created });
            continue;
        }
        Change modified { .handle = object.handle, .what = Change::What::Modified };
        for (const auto & [name, parameter] : object.parameters) {
            if (parameter.version > since) {
                modified.parameters.push_back(name);
            }
        }
        if (!modified.parameters.empty()) {
            found.emplace_back(object.handle.index, std::move(modified));
        }
    }

    // by index, and at one index a removal before what was made there after it
    std::stable_sort(found.begin(), found.end(), [](const auto & left, const auto & right) { return left.first < right.first; });
    std::vector<Change> ordered;
    ordered.reserve(found.size());
    for (auto & [index, change] : found) {
        ordered.push_back(std::move(change));
    }
    return ordered;
}

void Scene::forget(std::uint64_t through) {
    std::erase_if(removals_, [through](const auto & removal) { return removal.first <= through; });
}

};  // namespace v3d::moya::scene

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/scene/Schema.h>

#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

namespace v3d::moya::scene {

const char * name(Interpolation interpolation) {
    switch (interpolation) {
        case Interpolation::Constant: return "constant";
        case Interpolation::Uniform: return "uniform";
        case Interpolation::Varying: return "varying";
        case Interpolation::Vertex: return "vertex";
        case Interpolation::FaceVarying: return "facevarying";
    }
    return "unknown";
}

void Declaration::admit(const Value & value) const {
    if (count != kAny && value.size() != count) {
        throw std::invalid_argument("parameter " + name + " holds " + std::to_string(count) + " elements, not " + std::to_string(value.size()));
    }
    if (choices.empty()) {
        return;
    }
    for (const std::string & chosen : value.stringArray()) {
        if (std::find(choices.begin(), choices.end(), chosen) == choices.end()) {
            throw std::invalid_argument("parameter " + name + " cannot be " + chosen);
        }
    }
}

void Schema::check(const Declaration & declaration) const {
    if (declaration.name.empty()) {
        throw std::invalid_argument("a parameter with no name");
    }
    if (declaration.fallback.type() != declaration.type) {
        throw std::invalid_argument("parameter " + declaration.name + " is a " + name(declaration.type) + " with a fallback of type " +
            name(declaration.fallback.type()));
    }
    if (!declaration.choices.empty() && declaration.type != Type::String) {
        throw std::invalid_argument("parameter " + declaration.name + " has choices but is not a string");
    }
    declaration.admit(declaration.fallback);
    if (declaration.type == Type::Handle) {
        if (declaration.targets.empty()) {
            throw std::invalid_argument("handle parameter " + declaration.name + " names no kind of object");
        }
        for (const std::string & target : declaration.targets) {
            if (!find(target)) {
                throw std::invalid_argument("parameter " + declaration.name + " names no kind of object: " + target);
            }
        }
    } else if (!declaration.targets.empty()) {
        throw std::invalid_argument("parameter " + declaration.name + " names a kind but is not a handle");
    }
}

std::uint32_t Schema::add(const Kind & kind) {
    if (kind.name.empty()) {
        throw std::invalid_argument("a kind with no name");
    }
    if (find(kind.name)) {
        throw std::invalid_argument("two kinds named " + kind.name);
    }
    for (std::size_t i = 0; i < kind.parameters.size(); ++i) {
        check(kind.parameters[i]);
        for (std::size_t before = 0; before < i; ++before) {
            if (kind.parameters[before].name == kind.parameters[i].name) {
                throw std::invalid_argument("kind " + kind.name + " has two parameters named " + kind.parameters[i].name);
            }
        }
    }
    kinds_.push_back(kind);
    return static_cast<std::uint32_t>(kinds_.size() - 1);
}

const Kind & Schema::kind(std::uint32_t index) const {
    return kinds_.at(index);
}

std::optional<std::uint32_t> Schema::find(const std::string & name) const {
    for (std::size_t i = 0; i < kinds_.size(); ++i) {
        if (kinds_[i].name == name) {
            return static_cast<std::uint32_t>(i);
        }
    }
    return std::nullopt;
}

std::uint32_t Schema::size() const noexcept {
    return static_cast<std::uint32_t>(kinds_.size());
}

};  // namespace v3d::moya::scene

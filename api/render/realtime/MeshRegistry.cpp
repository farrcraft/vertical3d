/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MeshRegistry.h"

#include <api/asset/Manager.h>
#include <api/asset/Worker.h>
#include <api/image/Image.h>
#include <api/render/realtime/DeviceContext.h>

#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/weak_ptr.hpp>

namespace v3d::render::realtime {

// the layout a pipeline drawing an entry declares, which nothing at runtime checks
static_assert(sizeof(type::Model::Vertex) == 32, "a model vertex is a position, a normal and a uv");
static_assert(offsetof(type::Model::Vertex, position) == 0, "the position is at location 0");
static_assert(offsetof(type::Model::Vertex, normal) == 12, "the normal is at location 1");
static_assert(offsetof(type::Model::Vertex, uv) == 24, "the uv is at location 2");
// and a skinned one's, which renderer::Lit's skinned pipelines declare
static_assert(sizeof(MeshRegistry::SkinnedVertex) == 56, "a skinned vertex is a model vertex and its influence");
static_assert(offsetof(MeshRegistry::SkinnedVertex, influence) == 32, "the influence follows the vertex");
static_assert(offsetof(type::Model::Influence, joints) == 0, "the joints are at location 3");
static_assert(offsetof(type::Model::Influence, weights) == 8, "the weights are at location 4");

namespace {

/**
 * The mesh on the device: the model's vertices as they are, or interleaved with their
 * influences for a model with a skeleton.
 **/
boost::shared_ptr<vulkan::memory::Mesh> device(const boost::shared_ptr<DeviceContext>& context, const type::Model& model) {
    if (model.skeleton().empty()) {
        return boost::make_shared<vulkan::memory::Mesh>(context->device(), context->uploader(), model.vertices().data(),
            static_cast<VkDeviceSize>(model.vertexBytes()), static_cast<uint32_t>(model.vertices().size()),
            model.indices().data(), static_cast<uint32_t>(model.indices().size()));
    }
    std::vector<MeshRegistry::SkinnedVertex> vertices(model.vertices().size());
    for (std::size_t index = 0; index < vertices.size(); index++) {
        vertices[index].vertex = model.vertices()[index];
        vertices[index].influence = model.influences()[index];
    }
    return boost::make_shared<vulkan::memory::Mesh>(context->device(), context->uploader(), vertices.data(),
        static_cast<VkDeviceSize>(vertices.size() * sizeof(MeshRegistry::SkinnedVertex)), static_cast<uint32_t>(vertices.size()),
        model.indices().data(), static_cast<uint32_t>(model.indices().size()));
}

/**
 * What an exception says, for the line a failed request is reported on.
 **/
std::string describe(const std::exception_ptr& error) {
    try {
        std::rethrow_exception(error);
    } catch (const std::exception& exception) {
        return exception.what();
    } catch (...) {
        return "an error that is not a std::exception";
    }
}

/**
 * The bytes a decoded model puts on the device: its vertices, its indices, and each albedo it
 * carries pixels for that is not already uploaded, once.
 **/
std::size_t bytes(const DecodedModel& decoded, const AlbedoUploaded& uploaded) {
    std::size_t total = decoded.model->vertexBytes() + decoded.model->indices().size() * sizeof(std::uint32_t);
    std::set<const image::Image*> counted;
    for (const DecodedModel::Albedo& albedo : decoded.albedos) {
        if (albedo.pixels && !uploaded(albedo.key) && counted.insert(albedo.pixels.get()).second) {
            total += static_cast<std::size_t>(albedo.pixels->width()) * albedo.pixels->height() * albedo.pixels->bpp() / 8;
        }
    }
    return total;
}

};  // namespace

/**
 **/
MeshRegistry::MeshRegistry(const boost::shared_ptr<log::Logger>& logger, const boost::shared_ptr<DeviceContext>& context,
    const boost::shared_ptr<asset::Manager>& assets, const boost::shared_ptr<asset::Worker>& worker) :
    logger_(logger),
    context_(context),
    assets_(assets),
    worker_(worker),
    inbox_(boost::make_shared<Inbox>()) {
}

/**
 **/
MeshHandle MeshRegistry::load(const std::string& path) {
    const std::optional<std::pair<MeshHandle, bool>> known = registered(path);
    if (known && !known->second) {
        return known->first;
    }

    const DecodedModel decoded = decodeModel(*assets_, path, [this](const std::string& key) { return uploaded(key); });
    return upload(decoded.path, *decoded.model, decoded.albedos, known ? known->first : MeshHandle());
}

/**
 **/
MeshHandle MeshRegistry::request(const std::string& path) {
    if (!worker_) {
        throw std::logic_error("The mesh registry has no worker, so " + path + " cannot be requested");
    }
    const std::optional<std::pair<MeshHandle, bool>> known = registered(path);
    if (known) {
        return known->first;
    }

    Slot slot;
    slot.key = path;
    slot.pending = true;
    const MeshHandle handle = meshes_.add(slot);
    keys_[path] = handle;
    pending_++;
    try {
        worker_->post([assets = assets_, path]() { return decodeModel(*assets, path); },
            [inbox = boost::weak_ptr<Inbox>(inbox_), handle](std::expected<DecodedModel, std::exception_ptr> result) {
                const boost::shared_ptr<Inbox> held = inbox.lock();
                if (held) {
                    held->push_back(Arrival{ handle, std::move(result) });
                }
            });
    } catch (...) {
        abandon(handle);
        throw;
    }
    return handle;
}

/**
 **/
std::size_t MeshRegistry::poll(std::size_t budget) {
    std::size_t count = 0;
    std::size_t spent = 0;
    while (!inbox_->empty() && (count == 0 || spent < budget)) {
        Arrival arrival = std::move(inbox_->front());
        inbox_->pop_front();

        // a request released, or loaded at once, before its decoding finished has no slot to fill
        const Slot* slot = meshes_.resolve(arrival.handle);
        if (slot == nullptr || !slot->pending) {
            continue;
        }
        const std::string key = slot->key;
        if (!arrival.result) {
            logger_->get()->error("The model {} could not be loaded: {}", key, describe(arrival.result.error()));
            abandon(arrival.handle);
            continue;
        }

        const DecodedModel& decoded = *arrival.result;
        // counted before the upload, which makes every albedo it names an uploaded one
        const std::size_t cost = bytes(decoded, [this](const std::string& albedo) { return uploaded(albedo); });
        try {
            upload(key, *decoded.model, decoded.albedos, arrival.handle);
        } catch (const std::exception& error) {
            logger_->get()->error("The model {} could not be uploaded: {}", key, error.what());
            abandon(arrival.handle);
            continue;
        }
        count++;
        spent += cost;
    }
    return count;
}

/**
 **/
MeshHandle MeshRegistry::add(const std::string& name, const type::Model& model,
    const std::vector<boost::shared_ptr<image::Image>>& albedos) {
    const std::optional<std::pair<MeshHandle, bool>> known = registered(name);
    if (known && !known->second) {
        return known->first;
    }

    std::vector<DecodedModel::Albedo> sources(model.materials().size());
    for (std::size_t index = 0; index < sources.size(); index++) {
        if (index < albedos.size() && albedos[index]) {
            sources[index] = DecodedModel::Albedo{ name + "#albedo" + std::to_string(index), albedos[index] };
        } else {
            sources[index].key = model.materials()[index].baseColourTexture;
        }
    }
    decodeAlbedos(*assets_, &sources, [this](const std::string& key) { return uploaded(key); });
    return upload(name, model, sources, known ? known->first : MeshHandle());
}

/**
 **/
MeshHandle MeshRegistry::add(const DecodedModel& decoded) {
    const std::optional<std::pair<MeshHandle, bool>> known = registered(decoded.path);
    if (known && !known->second) {
        return known->first;
    }
    if (!decoded.model) {
        throw std::runtime_error("The model " + decoded.path + " holds no geometry to upload");
    }
    return upload(decoded.path, *decoded.model, decoded.albedos, known ? known->first : MeshHandle());
}

/**
 **/
MeshHandle MeshRegistry::upload(const std::string& key, const type::Model& model, const std::vector<DecodedModel::Albedo>& albedos,
    const MeshHandle& pending) {
    if (model.empty() || model.indices().empty()) {
        throw std::runtime_error("The model " + key + " holds no geometry to upload");
    }
    if (model.parts().empty()) {
        throw std::runtime_error("The model " + key + " has no parts to draw");
    }
    if (!model.skeleton().empty() && model.influences().size() != model.vertices().size()) {
        throw std::runtime_error("The model " + key + " has a skeleton and not an influence for every vertex");
    }
    if (albedos.size() != model.materials().size()) {
        throw std::runtime_error("The model " + key + " does not have one albedo per material");
    }
    // checked before anything is acquired, so that a bad part acquires nothing
    for (const type::Model::Part& part : model.parts()) {
        if (part.material >= model.materials().size() ||
            static_cast<std::size_t>(part.firstIndex) + part.indexCount > model.indices().size()) {
            throw std::runtime_error("The model " + key + " has a part outside its indices or its materials");
        }
    }

    Slot slot;
    slot.key = key;
    slot.entry.mesh = device(context_, model);
    if (!model.skeleton().empty()) {
        slot.entry.skin = boost::make_shared<const Skin>(Skin{ model.skeleton(), model.clips() });
    }
    MeshHandle handle = pending;
    try {
        for (const type::Model::Part& source : model.parts()) {
            Part part;
            part.firstIndex = source.firstIndex;
            part.indexCount = source.indexCount;
            part.baseColour = model.materials()[source.material].baseColour;
            slot.albedos.push_back(acquire(albedos[source.material], &part));
            slot.entry.parts.push_back(part);
        }
        if (pending.valid()) {
            Slot* requested = meshes_.resolve(pending);
            if (requested == nullptr || !requested->pending) {
                throw std::logic_error("The model " + key + " was to fill a request that is gone");
            }
            *requested = slot;
        } else {
            handle = meshes_.add(slot);
        }
    } catch (...) {
        // an albedo can fail to load or upload part way through, and the ones acquired for the
        // earlier parts are given back, so a failed upload leaves no texture counted
        for (const std::string& acquired : slot.albedos) {
            if (!acquired.empty()) {
                drop(acquired);
            }
        }
        throw;
    }
    if (pending.valid()) {
        pending_--;
    }
    keys_[key] = handle;
    return handle;
}

/**
 **/
std::string MeshRegistry::acquire(const DecodedModel::Albedo& source, Part* part) {
    const boost::shared_ptr<Textures> textures = context_->textures();
    part->texture = textures->white();
    part->material = textures->material(part->texture);
    const std::string& key = source.key;
    if (key.empty()) {
        return std::string();
    }

    std::map<std::string, Albedo>::iterator found = albedos_.find(key);
    if (found == albedos_.end()) {
        const boost::shared_ptr<image::Image>& image = source.pixels;
        if (!image) {
            logger_->get()->warn("The albedo {} was not found, and is drawn white", key);
            return std::string();
        }

        Albedo albedo;
        // a lit albedo is decoded to linear before it is lit
        albedo.texture = textures->texture(image, vulkan::memory::TextureFactory::Encoding::Srgb);
        try {
            albedo.material = textures->material(albedo.texture);
        } catch (...) {
            // the texture is not counted yet, so nothing else would release it
            textures->release(albedo.texture);
            throw;
        }
        albedo.users = 0;
        found = albedos_.emplace(key, albedo).first;
    }

    found->second.users++;
    part->texture = found->second.texture;
    part->material = found->second.material;
    return key;
}

/**
 **/
bool MeshRegistry::uploaded(const std::string& key) const {
    return albedos_.find(key) != albedos_.end();
}

/**
 **/
std::optional<std::pair<MeshHandle, bool>> MeshRegistry::registered(const std::string& key) const {
    const std::map<std::string, MeshHandle>::const_iterator found = keys_.find(key);
    if (found == keys_.end()) {
        return std::nullopt;
    }
    const Slot* slot = meshes_.resolve(found->second);
    return std::make_pair(found->second, slot != nullptr && slot->pending);
}

/**
 **/
void MeshRegistry::abandon(const MeshHandle& handle) {
    const std::optional<Slot> released = meshes_.release(handle);
    if (released) {
        keys_.erase(released->key);
        pending_--;
    }
}

/**
 **/
void MeshRegistry::drop(const std::string& key) {
    const std::map<std::string, Albedo>::iterator found = albedos_.find(key);
    if (found == albedos_.end()) {
        return;
    }
    found->second.users--;
    if (found->second.users == 0) {
        // the texture and its material go together, each once no frame can still read it
        context_->textures()->release(found->second.texture);
        albedos_.erase(found);
    }
}

/**
 **/
bool MeshRegistry::release(const MeshHandle& handle) {
    std::optional<Slot> released = meshes_.release(handle);
    if (!released) {
        return false;
    }
    keys_.erase(released->key);
    if (released->pending) {
        // nothing is on the device yet, and what the worker decodes for it is dropped
        pending_--;
        return true;
    }
    for (const std::string& albedo : released->albedos) {
        drop(albedo);
    }
    // a frame in flight may still be drawing it
    context_->ring()->retire([mesh = released->entry.mesh]() mutable { mesh.reset(); });
    return true;
}

/**
 **/
const MeshRegistry::Entry* MeshRegistry::resolve(const MeshHandle& handle) const {
    const Slot* slot = meshes_.resolve(handle);
    return slot != nullptr && !slot->pending ? &slot->entry : nullptr;
}

/**
 **/
bool MeshRegistry::pending(const MeshHandle& handle) const {
    const Slot* slot = meshes_.resolve(handle);
    return slot != nullptr && slot->pending;
}

/**
 **/
std::optional<uint32_t> MeshRegistry::clip(const MeshHandle& handle, const std::string& name) const {
    const Entry* entry = resolve(handle);
    if (entry == nullptr || !entry->skin) {
        return std::nullopt;
    }
    for (std::size_t index = 0; index < entry->skin->clips.size(); index++) {
        if (entry->skin->clips[index].name == name) {
            return static_cast<uint32_t>(index);
        }
    }
    return std::nullopt;
}

/**
 **/
std::size_t MeshRegistry::count() const noexcept {
    return keys_.size() - pending_;
}

/**
 **/
std::size_t MeshRegistry::pending() const noexcept {
    return pending_;
}

};  // namespace v3d::render::realtime

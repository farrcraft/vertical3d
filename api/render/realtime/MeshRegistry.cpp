/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MeshRegistry.h"

#include <api/asset/Manager.h>
#include <api/asset/Worker.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/Mesh.h>
#include <api/render/realtime/vulkan/memory/TextureFactory.h>
#include <api/type/Model.h>

#include "DecodedModel.h"
#include "Handle.h"
#include "Textures.h"
#include "UploadQueue.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime {

// the layout a pipeline drawing an entry declares, which nothing at runtime checks: each
// member follows the one before it with no padding
static_assert(sizeof(type::Model::Vertex) == 32, "a model vertex is a position, a normal and a uv");
static_assert(offsetof(type::Model::Vertex, position) == 0, "the position is at location 0");
static_assert(offsetof(type::Model::Vertex, normal) == sizeof(glm::vec3), "the normal is at location 1");
static_assert(offsetof(type::Model::Vertex, uv) == 2 * sizeof(glm::vec3), "the uv is at location 2");
// and a skinned one's, which renderer::Lit's skinned pipelines declare
static_assert(sizeof(MeshRegistry::SkinnedVertex) == sizeof(type::Model::Vertex) + sizeof(type::Model::Influence),
    "a skinned vertex is a model vertex and its influence");
static_assert(offsetof(MeshRegistry::SkinnedVertex, influence) == 32, "the influence follows the vertex");
static_assert(offsetof(type::Model::Influence, joints) == 0, "the joints are at location 3");
static_assert(offsetof(type::Model::Influence, weights) == 8, "the weights are at location 4");

namespace {

/**
 * The mesh on the device: the model's vertices as they are, or interleaved with their
 * influences for a model with a skeleton.
 **/
boost::shared_ptr<Mesh> device(const boost::shared_ptr<DeviceContext>& context, const type::Model& model) {
    if (model.skeleton().empty()) {
        return boost::make_shared<Mesh>(context->device(), context->uploader(),
            std::as_bytes(std::span(model.vertices())), static_cast<uint32_t>(model.vertices().size()),
            std::span<const uint32_t>(model.indices()));
    }
    std::vector<MeshRegistry::SkinnedVertex> vertices(model.vertices().size());
    for (std::size_t index = 0; index < vertices.size(); index++) {
        vertices[index].vertex = model.vertices()[index];
        vertices[index].influence = model.influences()[index];
    }
    return boost::make_shared<Mesh>(context->device(), context->uploader(),
        std::as_bytes(std::span(vertices)), static_cast<uint32_t>(vertices.size()), std::span<const uint32_t>(model.indices()));
}

/**
 * The bytes a decoded model puts on the device: its vertices, its indices, and each albedo it
 * carries pixels for that is not already uploaded, once.
 **/
std::size_t bytes(const DecodedModel& decoded, const AlbedoUploaded& uploaded) {
    std::size_t total = decoded.model->vertexBytes() + (decoded.model->indices().size() * sizeof(std::uint32_t));
    std::set<const image::Image*> counted;
    for (const DecodedModel::Albedo& albedo : decoded.albedos) {
        if (albedo.pixels && !uploaded(albedo.key) && counted.insert(albedo.pixels.get()).second) {
            total += static_cast<std::size_t>(albedo.pixels->width()) * albedo.pixels->height() * albedo.pixels->bpp() / 8;
        }
    }
    return total;
}

/**
 * A sphere around a model's vertices: the middle of the box that holds them, and the farthest
 * of them from it. A model with no vertices is a point at the origin.
 **/
glm::vec4 bound(const type::Model& model) {
    const std::vector<type::Model::Vertex>& vertices = model.vertices();
    if (vertices.empty()) {
        return glm::vec4(0.0F);
    }
    glm::vec3 lowest = vertices.front().position;
    glm::vec3 highest = lowest;
    for (const type::Model::Vertex& vertex : vertices) {
        lowest = glm::min(lowest, vertex.position);
        highest = glm::max(highest, vertex.position);
    }
    const glm::vec3 centre = (lowest + highest) * 0.5F;
    float radius = 0.0F;
    for (const type::Model::Vertex& vertex : vertices) {
        radius = std::max(radius, glm::length(vertex.position - centre));
    }
    return glm::vec4(centre, radius);
}

};  // namespace

/**
 **/
MeshRegistry::MeshRegistry(const boost::shared_ptr<log::Logger>& logger, const boost::shared_ptr<DeviceContext>& context,
    const boost::shared_ptr<asset::Manager>& assets, const boost::shared_ptr<asset::Worker>& worker) :
    logger_(logger),
    context_(context),
    assets_(assets),
    worker_(worker) {
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
        uploads_.post(worker_.get(), handle, [assets = assets_, path]() { return decodeModel(*assets, path); });
    } catch (...) {
        abandon(handle);
        throw;
    }
    return handle;
}

/**
 **/
std::size_t MeshRegistry::poll(std::size_t budget) {
    return uploads_.poll(budget,
        [this](const MeshHandle& handle) {
            // a request released, or loaded at once, before its decoding finished has no slot to
            // fill
            const Slot* slot = meshes_.resolve(handle);
            return slot != nullptr && slot->pending;
        },
        [this](const MeshHandle& handle, const DecodedModel& decoded) {
            // counted before the upload, which makes every albedo it names an uploaded one
            const std::size_t cost = bytes(decoded, [this](const std::string& albedo) { return uploaded(albedo); });
            const std::string key = meshes_.resolve(handle)->key;
            upload(key, *decoded.model, decoded.albedos, handle);
            return cost;
        },
        [this](const MeshHandle& handle, const std::string& reason) {
            logger_->get()->error("The model {} could not be loaded: {}", meshes_.resolve(handle)->key, reason);
            // the handle resolves to nothing from now on, and the path is free for a request that
            // reads the file again
            abandon(handle);
        });
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
    slot.entry.bound = bound(model);
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
    part->material = textures->material();
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
        albedo.users = 0;
        found = albedos_.emplace(key, albedo).first;
    }

    found->second.users++;
    part->texture = found->second.texture;
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
        // the texture and its slot go together, once no frame can still read them
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

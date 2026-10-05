/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MeshRegistry.h"

#include <api/asset/Manager.h>
#include <api/asset/media/kind/Image.h>
#include <api/asset/media/kind/Model.h>
#include <api/image/Image.h>
#include <api/render/realtime/DeviceContext.h>

#include <cstddef>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>

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

};  // namespace

/**
 **/
MeshRegistry::MeshRegistry(const boost::shared_ptr<log::Logger>& logger, const boost::shared_ptr<DeviceContext>& context,
    const boost::shared_ptr<asset::Manager>& assets) :
    logger_(logger),
    context_(context),
    assets_(assets) {
}

/**
 **/
MeshHandle MeshRegistry::load(const std::string& path) {
    const std::map<std::string, MeshHandle>::const_iterator found = keys_.find(path);
    if (found != keys_.end()) {
        return found->second;
    }

    const boost::shared_ptr<asset::media::kind::Model> loaded =
        assets_->load<asset::media::kind::Model>(path, asset::Type::ModelGltf);
    if (!loaded || !loaded->model()) {
        throw std::runtime_error("Unable to load a model from " + path);
    }
    const type::Model& model = *loaded->model();

    // an image packed into the file is the file's own, and one it names is resolved beside it
    std::vector<Source> sources(model.materials().size());
    for (std::size_t index = 0; index < sources.size(); index++) {
        const std::string& named = model.materials()[index].baseColourTexture;
        if (loaded->baseColourImage(index)) {
            sources[index] = Source{ path + "#albedo" + std::to_string(index), loaded->baseColourImage(index) };
        } else if (!named.empty()) {
            sources[index].key = (boost::filesystem::path(path).parent_path() / named).generic_string();
        }
    }
    return upload(path, model, sources);
}

/**
 **/
MeshHandle MeshRegistry::add(const std::string& name, const type::Model& model,
    const std::vector<boost::shared_ptr<image::Image>>& albedos) {
    const std::map<std::string, MeshHandle>::const_iterator found = keys_.find(name);
    if (found != keys_.end()) {
        return found->second;
    }

    std::vector<Source> sources(model.materials().size());
    for (std::size_t index = 0; index < sources.size(); index++) {
        if (index < albedos.size() && albedos[index]) {
            sources[index] = Source{ name + "#albedo" + std::to_string(index), albedos[index] };
        } else {
            sources[index].key = model.materials()[index].baseColourTexture;
        }
    }
    return upload(name, model, sources);
}

/**
 **/
MeshHandle MeshRegistry::upload(const std::string& key, const type::Model& model, const std::vector<Source>& sources) {
    if (model.empty() || model.indices().empty()) {
        throw std::runtime_error("The model " + key + " holds no geometry to upload");
    }
    if (model.parts().empty()) {
        throw std::runtime_error("The model " + key + " has no parts to draw");
    }
    if (!model.skeleton().empty() && model.influences().size() != model.vertices().size()) {
        throw std::runtime_error("The model " + key + " has a skeleton and not an influence for every vertex");
    }
    // checked before anything is acquired, so that a bad part leaves no albedo counted
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
    for (const type::Model::Part& source : model.parts()) {
        Part part;
        part.firstIndex = source.firstIndex;
        part.indexCount = source.indexCount;
        part.baseColour = model.materials()[source.material].baseColour;
        slot.albedos.push_back(acquire(sources[source.material], &part));
        slot.entry.parts.push_back(part);
    }

    const MeshHandle handle = meshes_.add(slot);
    keys_[key] = handle;
    return handle;
}

/**
 **/
std::string MeshRegistry::acquire(const Source& source, Part* part) {
    const boost::shared_ptr<Textures> textures = context_->textures();
    part->texture = textures->white();
    part->material = textures->material(part->texture);
    const std::string& key = source.key;
    if (key.empty()) {
        return std::string();
    }

    std::map<std::string, Albedo>::iterator found = albedos_.find(key);
    if (found == albedos_.end()) {
        boost::shared_ptr<image::Image> image = source.pixels;
        if (!image) {
            const boost::shared_ptr<asset::media::kind::Image> asset = assets_->load<asset::media::kind::Image>(key);
            if (asset) {
                image = asset->image();
            }
        }
        if (!image) {
            logger_->get()->warn("The albedo {} was not found, and is drawn white", key);
            return std::string();
        }

        Albedo albedo;
        // a lit albedo is decoded to linear before it is lit - ADR-0066
        albedo.texture = textures->texture(image, vulkan::memory::TextureFactory::Encoding::Srgb);
        albedo.material = textures->material(albedo.texture);
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
    return slot != nullptr ? &slot->entry : nullptr;
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
    return keys_.size();
}

};  // namespace v3d::render::realtime

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Registry.h>
#include <api/render/realtime/vulkan/memory/Mesh.h>
#include <api/type/Model.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::asset {
class Manager;
};  // namespace v3d::asset

namespace v3d::image {
class Image;
};  // namespace v3d::image

namespace v3d::render::realtime {

class DeviceContext;

/**
 * Models on the device, each uploaded once and named by handle.
 *
 * A model loaded from a path, or added under a name, is uploaded the first time and handed
 * back by the same handle every time after, so a hundred props drawn from one file are one
 * upload. A model is one vertex buffer drawn a part at a time. Each part's albedo is a
 * texture and a material from the context's Textures, shared by every part naming the same
 * image, and the white texture for a part naming none.
 *
 * The vertex layout is type::Model::Vertex - a position, a normal and a uv, 32 bytes - which
 * is what a pipeline drawing an entry declares. A model with a skeleton is uploaded as a
 * SkinnedVertex instead, and keeps its skeleton and clips on the CPU for whatever poses it.
 *
 * A handle is released explicitly, and resolves to nothing at once. An albedo is released with
 * the last entry naming it. Items queued before a release are still drawn by their frame, with
 * their albedo, and the mesh is destroyed once that frame has finished.
 **/
class MeshRegistry final {
 public:
    /**
     * A range of the mesh's indices and the surface it is drawn with: one draw.
     **/
    struct Part final {
        uint32_t firstIndex;
        uint32_t indexCount;
        TextureHandle texture;   /**< the albedo, or the white texture **/
        MaterialHandle material; /**< what binds the albedo at set 1 **/
        glm::vec4 baseColour;    /**< what the albedo is multiplied by **/
    };

    /**
     * A skinned model's vertex on the device: the model's vertex, then the joints that move it -
     * what renderer::Lit's skinned pipelines read.
     **/
    struct SkinnedVertex final {
        type::Model::Vertex vertex;
        type::Model::Influence influence;
    };

    /**
     * What a skinned entry is posed by, shared by every entity drawing it.
     **/
    struct Skin final {
        type::Skeleton skeleton;
        std::vector<type::animation::Clip> clips;
    };

    /**
     * What a handle resolves to: everything a draw of the model needs except where it is.
     **/
    struct Entry final {
        boost::shared_ptr<vulkan::memory::Mesh> mesh;
        std::vector<Part> parts;  /**< in the model's order, which is the order they are drawn **/
        boost::shared_ptr<const Skin> skin;  /**< null for a static model **/
    };

    /**
     * @param logger where a texture that could not be found is reported
     * @param context the device to upload to, and the textures an albedo is registered with
     * @param assets where a path is loaded from, and a texture a model names is resolved
     **/
    MeshRegistry(const boost::shared_ptr<log::Logger>& logger, const boost::shared_ptr<DeviceContext>& context,
        const boost::shared_ptr<asset::Manager>& assets);

    /**
     * Destroys every mesh still registered at once, the way Resources destroys what it holds,
     * so it goes after the frames in flight have finished. An albedo still registered with
     * the context's Textures stays there until the context goes.
     **/
    ~MeshRegistry() = default;

    MeshRegistry(const MeshRegistry&) = delete;
    MeshRegistry& operator=(const MeshRegistry&) = delete;

    /**
     * A glTF model, uploaded the first time its path is asked for.
     *
     * A texture a material names is resolved beside the file, through the asset manager. One
     * that cannot be found is reported and drawn white, so a missing image is a wrong picture
     * rather than a scene that does not load.
     *
     * @param path relative to the asset manager's root
     * @throw std::runtime_error if the file is not a model, or holds no geometry
     **/
    MeshHandle load(const std::string& path);

    /**
     * A model built in code, uploaded the first time its name is asked for.
     *
     * @param name what the model is registered under, sharing a namespace with load()'s paths
     * @param model the geometry, its parts and their materials. A texture a material names is
     *        resolved from the asset manager's root
     * @param albedos pixels for each material's albedo, by the material's index, where the
     *        caller has them rather than a name. Null or missing for a material that has none
     * @throw std::runtime_error if the model holds no geometry, a part outside it, or a skeleton
     *        without an influence for every vertex
     **/
    MeshHandle add(const std::string& name, const type::Model& model,
        const std::vector<boost::shared_ptr<image::Image>>& albedos = {});

    /**
     * Stop addressing an entry. Items queued before the release are still drawn by their frame,
     * with their albedo. What only this entry was using is destroyed once that frame has
     * finished.
     *
     * @return whether the handle referred to anything
     **/
    bool release(const MeshHandle& handle);

    /**
     * @return the entry, or nullptr for a handle that was released or never given out
     **/
    const Entry* resolve(const MeshHandle& handle) const;

    /**
     * @return the index of an entry's clip of that name, which ecs::component::play() takes, or
     *         nothing when the entry has no skin or no such clip
     **/
    std::optional<uint32_t> clip(const MeshHandle& handle, const std::string& name) const;

    /**
     * @return how many entries are registered, which is how many models are on the device
     **/
    std::size_t count() const noexcept;

 private:
    /**
     * An entry, and the keys it was registered under so that releasing it can find them.
     **/
    struct Slot final {
        Entry entry;
        std::string key;
        std::vector<std::string> albedos;  /**< each part's albedo key, or empty for the white texture **/
    };

    /**
     * Where a material's albedo comes from: the key it is shared by, and its pixels when the
     * caller has them rather than a name to load.
     **/
    struct Source final {
        std::string key;  /**< empty for a material with no albedo **/
        boost::shared_ptr<image::Image> pixels;
    };

    /**
     * An albedo, and how many entries name it.
     **/
    struct Albedo final {
        TextureHandle texture;
        MaterialHandle material;
        uint32_t users;
    };

    /**
     * Upload a model and register it under a key, with each part's albedo found from its
     * material's source.
     *
     * @param sources one per material, by index
     * @throw std::runtime_error if the model holds no geometry, no parts, or a part outside it
     **/
    MeshHandle upload(const std::string& key, const type::Model& model, const std::vector<Source>& sources);

    /**
     * Find or upload an albedo, and count one more user of it.
     *
     * @return the key it is shared by, or empty when it could not be found and white is used
     **/
    std::string acquire(const Source& source, Part* part);

    /**
     * Count one fewer user of an albedo, and release it when none is left.
     **/
    void drop(const std::string& key);

    boost::shared_ptr<log::Logger> logger_;
    boost::shared_ptr<DeviceContext> context_;
    boost::shared_ptr<asset::Manager> assets_;
    Registry<MeshTag, Slot> meshes_;
    std::map<std::string, MeshHandle> keys_;
    std::map<std::string, Albedo> albedos_;
};

};  // namespace v3d::render::realtime

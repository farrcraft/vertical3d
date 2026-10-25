/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/DecodedModel.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Registry.h>
#include <api/render/realtime/vulkan/memory/Mesh.h>
#include <api/type/Model.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <expected>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::asset {
class Manager;
class Worker;
};  // namespace v3d::asset

namespace v3d::image {
class Image;
};  // namespace v3d::image

namespace v3d::render::realtime {

class DeviceContext;

/**
 * Models on the device, each uploaded once and named by handle.
 *
 * A model loaded from a path, or added under a name, is uploaded the first time. Every later
 * request for it returns the same handle, so a hundred props drawn from one file are one
 * upload. A model is one vertex buffer drawn a part at a time. Each part's albedo is a
 * texture from the context's Textures, shared by every part naming the same image, and the
 * white texture for a part naming none. A draw reads it by the texture's slot.
 *
 * The vertex layout is type::Model::Vertex: a position, a normal and a uv, 32 bytes. A pipeline
 * that draws an entry declares that layout. A model with a skeleton is uploaded as a
 * SkinnedVertex instead, and keeps its skeleton and clips on the CPU for whatever poses it.
 *
 * A handle is released explicitly, and resolves to nothing at once. An albedo is released with
 * the last entry naming it. Items queued before a release are still drawn by their frame, with
 * their albedo, and the mesh is destroyed once that frame has finished.
 *
 * A path can also be requested. The handle comes back at once and resolves to nothing while the
 * model is decoded on a worker thread. poll() uploads it, and from then on the handle resolves
 * like any other. A draw skips a handle that resolves to nothing, so a model still loading is
 * not drawn.
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
        MaterialHandle material; /**< what binds the texture array at set 1, which holds the albedo **/
        glm::vec4 baseColour;    /**< what the albedo is multiplied by **/
    };

    /**
     * A skinned model's vertex on the device: the model's vertex, then the joints that move it.
     * renderer::Lit's skinned pipelines read this layout.
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
     * The most bytes poll() uploads by default before it leaves the rest for the next call.
     **/
    static constexpr std::size_t DEFAULT_UPLOAD_BUDGET = std::size_t{4} * 1024 * 1024;

    /**
     * @param logger where a texture that could not be found is reported
     * @param context the device to upload to, and the textures an albedo is registered with
     * @param assets where a path is loaded from, and a texture a model names is resolved
     * @param worker what request() decodes on. Without one, request() throws
     **/
    MeshRegistry(const boost::shared_ptr<log::Logger>& logger, const boost::shared_ptr<DeviceContext>& context,
        const boost::shared_ptr<asset::Manager>& assets, const boost::shared_ptr<asset::Worker>& worker = {});

    /**
     * Destroys every mesh still registered at once, as Resources does, so destroy the registry
     * only after the frames in flight have finished. An albedo still registered with the
     * context's Textures stays there until the context is destroyed.
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
     * A path already requested and not yet uploaded is read and uploaded now. What the worker
     * decodes for it later is dropped.
     *
     * @param path relative to the asset manager's root
     * @throw std::runtime_error if the file is not a model, or holds no geometry
     **/
    MeshHandle load(const std::string& path);

    /**
     * A glTF model, decoded on the worker thread and uploaded by a later poll().
     *
     * The handle resolves to nothing until then. Asking for a path again returns the same
     * handle, whether or not it is uploaded yet. A file that cannot be read or uploaded is
     * reported in the log, and its handle resolves to nothing from then on.
     *
     * @param path relative to the asset manager's root
     * @throw std::logic_error if the registry was made without a worker
     **/
    MeshHandle request(const std::string& path);

    /**
     * Upload the requested models whose decoding has finished, in the order they were requested.
     *
     * It uploads at least one when any is ready, and stops once the bytes uploaded reach the
     * budget. Call it once a frame, after polling the worker.
     *
     * @param budget the vertex, index and albedo bytes to upload before stopping. An albedo
     *        another entry already uploaded is not counted
     * @return how many models were uploaded
     **/
    std::size_t poll(std::size_t budget = DEFAULT_UPLOAD_BUDGET);

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
     * A model already read and decoded, uploaded the first time its path is asked for. An albedo
     * with a key and no pixels is drawn white unless an entry already uploaded it, and nothing is
     * read from a file.
     *
     * @throw std::runtime_error if the model holds no geometry, a part outside it, a skeleton
     *        without an influence for every vertex, or not one albedo per material
     **/
    MeshHandle add(const DecodedModel& decoded);

    /**
     * Stop addressing an entry. Items queued before the release are still drawn by their frame,
     * with their albedo. What only this entry was using is destroyed once that frame has
     * finished.
     *
     * @return whether the handle referred to anything
     **/
    bool release(const MeshHandle& handle);

    /**
     * @return the entry, or nullptr for a handle that was released, never given out, failed to
     *         load, or is requested and not yet uploaded
     **/
    const Entry* resolve(const MeshHandle& handle) const;

    /**
     * @return whether a handle is requested and not yet uploaded, so that a caller can tell one
     *         still loading from one that failed or was released
     **/
    bool pending(const MeshHandle& handle) const;

    /**
     * @return the index of an entry's clip of that name, which ecs::component::play() takes, or
     *         nothing when the entry has no skin or no such clip
     **/
    std::optional<uint32_t> clip(const MeshHandle& handle, const std::string& name) const;

    /**
     * @return how many entries are registered: one per model on the device, not counting those
     *         requested and not yet uploaded
     **/
    std::size_t count() const noexcept;

    /**
     * @return how many requests are not yet uploaded
     **/
    std::size_t pending() const noexcept;

 private:
    /**
     * An entry, and the keys it was registered under so that releasing it can find them.
     **/
    struct Slot final {
        Entry entry;
        std::string key;
        std::vector<std::string> albedos;  /**< each part's albedo key, or empty for the white texture **/
        bool pending = false;  /**< requested and not yet uploaded, so the entry is empty **/
    };

    /**
     * A requested model the worker has finished with, waiting for poll().
     **/
    struct Arrival final {
        MeshHandle handle;
        std::expected<DecodedModel, std::exception_ptr> result;
    };

    /**
     * What the worker's completions hand to this registry. A completion holds it weakly, so one
     * that runs after the registry is gone does nothing.
     **/
    using Inbox = std::deque<Arrival>;

    /**
     * An albedo, and how many entries name it.
     **/
    struct Albedo final {
        TextureHandle texture;
        uint32_t users;
    };

    /**
     * Upload a model and register it under a key, with each part's albedo taken from its
     * material's.
     *
     * @param albedos one per material, by index
     * @param pending the requested slot to fill, or an unset handle to take a new one
     * @throw std::runtime_error if the model holds no geometry, no parts, a part outside it, or
     *        not one albedo per material
     **/
    MeshHandle upload(const std::string& key, const type::Model& model, const std::vector<DecodedModel::Albedo>& albedos,
        const MeshHandle& pending = MeshHandle());

    /**
     * @return the handle registered under a key, and whether it is only requested, or nothing
     *         when the key is free
     **/
    std::optional<std::pair<MeshHandle, bool>> registered(const std::string& key) const;

    /**
     * Give up a request that could not be read or uploaded, so that its handle resolves to
     * nothing and its key is free for another request.
     **/
    void abandon(const MeshHandle& handle);

    /**
     * Find or upload an albedo, and count one more user of it. Nothing is read from a file.
     *
     * @return the key it is shared by, or empty when it has no pixels and none is uploaded, and
     *         white is used
     **/
    std::string acquire(const DecodedModel::Albedo& source, Part* part);

    /**
     * @return whether an albedo is uploaded under a key
     **/
    bool uploaded(const std::string& key) const;

    /**
     * Count one fewer user of an albedo, and release it when none is left.
     **/
    void drop(const std::string& key);

    boost::shared_ptr<log::Logger> logger_;
    boost::shared_ptr<DeviceContext> context_;
    boost::shared_ptr<asset::Manager> assets_;
    boost::shared_ptr<asset::Worker> worker_;
    boost::shared_ptr<Inbox> inbox_;
    Registry<MeshTag, Slot> meshes_;
    std::size_t pending_ = 0;
    std::map<std::string, MeshHandle> keys_;
    std::map<std::string, Albedo> albedos_;
};

};  // namespace v3d::render::realtime

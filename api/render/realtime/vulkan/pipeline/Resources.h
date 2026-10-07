/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Registry.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/Ring.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <set>

#include "Material.h"
#include "Pipeline.h"
#include "Texture.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::pipeline {

/**
 * Everything a draw item can name by handle, and the owner that destroys it.
 *
 * Draw items refer to resources by handle rather than by pointer so that the sort key
 * they carry is stable. That requires an owner that outlives the frames using a resource,
 * which this class is.
 *
 * A resource lives until it is released or the context goes. Pipelines are built at load time
 * and are not released. A released material or texture keeps its slot until every frame that
 * may name it has finished, so its handle is never reused early.
 **/
class Resources final {
 public:
    /**
     * @param device the device everything registered here was created on
     * @param ring the frames in flight, which hold back what is released until they finish
     **/
    Resources(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<frame::Ring>& ring);

    /**
     * Destroys every resource that was registered, in the order vulkan requires.
     **/
    ~Resources();

    Resources(const Resources&) = delete;
    Resources& operator=(const Resources&) = delete;

    /**
     * Take ownership of a pipeline and its layout.
     **/
    PipelineHandle add(const Pipeline& pipeline);

    /**
     * Take ownership of a material. The descriptor set belongs to the pool it was
     * allocated from, so only the handle is recorded here.
     **/
    MaterialHandle add(const Material& material);

    /**
     * Take ownership of a texture and everything backing it.
     **/
    TextureHandle add(const Texture& texture);

    /**
     * Stop addressing a material.
     *
     * Draw items queued before the release may name it, and are recorded into the frame
     * Ring::recording() names. The handle goes on resolving until that frame is submitted, and
     * resolves to nothing after. Its slot is freed once that frame has finished.
     *
     * The descriptor set belongs to the pool it came from, and whoever allocated it returns it
     * there.
     *
     * @return whether the handle referred to anything not already released
     **/
    bool release(const MaterialHandle& handle);

    /**
     * Stop addressing a texture.
     *
     * The handle resolves to nothing at once. A texture is resolved only to write a descriptor,
     * which may be bound after the frame. What it owns is destroyed, and its slot freed, once the
     * frame Ring::recording() names has finished.
     *
     * A material naming it has to be released as well, by whoever made it.
     *
     * @return whether the handle referred to anything not already released
     **/
    bool release(const TextureHandle& handle);

    /**
     * @return the pipeline the handle refers to, or nullptr
     **/
    const Pipeline* pipeline(const PipelineHandle& handle) const;

    /**
     * @return the material the handle refers to, or nullptr. A released material resolves
     *         until the frame its release was queued into is submitted.
     **/
    const Material* material(const MaterialHandle& handle) const;

    /**
     * @return the texture the handle refers to, or nullptr once it is released
     **/
    const Texture* texture(const TextureHandle& handle) const;

    /**
     * @return how many textures are held. A released texture is counted until it is destroyed,
     *         once the frames that may read it have finished.
     **/
    std::size_t textureCount() const noexcept;

 private:
    /**
     * What a release hands to the ring. The ring may run it after this object is destroyed, so
     * it is shared with every callback rather than reached through this.
     **/
    struct Held final {
        Registry<MaterialTag, Material> materials;
        Registry<TextureTag, Texture> textures;
        /**< each released material, and the last frame it may be recorded into **/
        std::map<MaterialHandle, uint64_t> retiringMaterials;
        std::set<TextureHandle> retiringTextures;
    };

    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<frame::Ring> ring_;
    Registry<PipelineTag, Pipeline> pipelines_;
    boost::shared_ptr<Held> held_;
};

};  // namespace v3d::render::realtime::vulkan::pipeline

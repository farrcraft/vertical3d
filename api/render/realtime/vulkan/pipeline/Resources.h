/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Registry.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/Ring.h>

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
 * A resource lives until it is released or the context goes. A released handle
 * resolves to nothing at once, and what it named is handed to the ring to destroy once the
 * frames that may still read it have finished. Pipelines are built at load time and are not
 * released.
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
     * Stop addressing a material. Its descriptor set belongs to the pool it came from, so
     * whoever allocated it decides what happens to the set.
     *
     * @return whether the handle referred to anything
     **/
    bool release(const MaterialHandle& handle);

    /**
     * Stop addressing a texture, and destroy what it owns once no frame in flight can still
     * be sampling it. A material naming it has to be released as well, by whoever made it.
     *
     * @return whether the handle referred to anything
     **/
    bool release(const TextureHandle& handle);

    /**
     * @return the pipeline the handle refers to, or nullptr
     **/
    const Pipeline* pipeline(const PipelineHandle& handle) const;

    /**
     * @return the material the handle refers to, or nullptr
     **/
    const Material* material(const MaterialHandle& handle) const;

    /**
     * @return the texture the handle refers to, or nullptr
     **/
    const Texture* texture(const TextureHandle& handle) const;

 private:
    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<frame::Ring> ring_;
    Registry<PipelineTag, Pipeline> pipelines_;
    Registry<MaterialTag, Material> materials_;
    Registry<TextureTag, Texture> textures_;
};

};  // namespace v3d::render::realtime::vulkan::pipeline

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Registry.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/memory/Image.h>

#include <vulkan/vulkan.h>

#include <vector>

#include "Sampler.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::pipeline {

/**
 * A graphics pipeline and the layout its descriptor sets and push constants are bound
 * through. Both are needed at record time, so they are registered together.
 **/
struct Pipeline final {
    Pipeline() noexcept;

    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkShaderStageFlags pushStages;  /**< which stages the layout declared push constants for **/
    bool scene;                     /**< whether the layout declares a set 2 - ADR-0064 **/
    bool biased;                    /**< whether depth bias is dynamic state, set per pass **/
    std::vector<VkFormat> colourFormats;  /**< what it was built to draw into - ADR-0068 **/
    VkFormat depthFormat;           /**< and its depth, or undefined for none **/
};

/**
 * What is bound at set 1 for a draw - the per material frequency of the binding
 * convention. A material outlives the frames that draw with it, so the set is allocated
 * once rather than per frame.
 **/
struct Material final {
    Material() noexcept;

    VkDescriptorSet set;
    TextureHandle texture;
};

/**
 * An image the shaders sample from, and the sampler they read it through.
 *
 * Both are shared rather than owned. A texture memory::TextureFactory built is the only
 * holder of its image; one registered from a frame::RenderTarget shares the target's, so
 * the image outlives whichever of the two lets go first. Either way a released texture is
 * destroyed by the last reference going, which the ring holds until no frame can read it.
 **/
struct Texture final {
    boost::shared_ptr<memory::Image> image;
    boost::shared_ptr<Sampler> sampler;
};

/**
 * Everything a draw item can name by handle, and the owner that destroys it.
 *
 * Draw items refer to resources by handle rather than by pointer so that the sort key
 * they carry means something - see ADR-0004. That only works while something outlives the
 * frames using a resource, which is what this is for.
 *
 * A resource lives until it is released or the context goes - ADR-0061. A released handle
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

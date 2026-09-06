/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include "Device.h"

#include "../Handle.h"
#include "../Registry.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan {

/**
 * A graphics pipeline and the layout its descriptor sets and push constants are bound
 * through. Both are needed at record time, so they are registered together.
 **/
struct Pipeline final {
    Pipeline() noexcept;

    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkShaderStageFlags pushStages;  /**< which stages the layout declared push constants for **/
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
 * An image the shaders sample from, with everything that has to be destroyed with it.
 **/
struct Texture final {
    Texture() noexcept;

    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkSampler sampler;
    VkExtent2D extent;

    /**
     * Whether registering this hands over the images or only names them.
     *
     * True for everything TextureFactory builds, which exists to be owned here. False for
     * a RenderTarget, whose images are the target's and are freed and reallocated whenever
     * it is resized - destroying them here as well would free them twice, and a target
     * outliving nothing is not what a handle into a registry means.
     **/
    bool owned;
};

/**
 * Everything a draw item can name by handle, and the owner that destroys it.
 *
 * Draw items refer to resources by handle rather than by pointer so that the sort key
 * they carry means something - see ADR-0004. That only works while a handle's slot is
 * stable and while something outlives the frames using it, which is what this is for.
 *
 * Resources live until the context does. Nothing here reference counts or frees an
 * individual resource: textures and pipelines are built at load time and used until the
 * app closes.
 **/
class Resources final {
 public:
    /**
     * @param device the device everything registered here was created on
     **/
    explicit Resources(const boost::shared_ptr<Device>& device);

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
    boost::shared_ptr<Device> device_;
    Registry<PipelineTag, Pipeline> pipelines_;
    Registry<MaterialTag, Material> materials_;
    Registry<TextureTag, Texture> textures_;
};

};  // namespace v3d::render::realtime::vulkan

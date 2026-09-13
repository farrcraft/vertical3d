/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Cache.h"
#include "Resources.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::pipeline {

/**
 * Describes a graphics pipeline a chained call at a time, and builds it.
 *
 * A VkGraphicsPipelineCreateInfo is a dozen substructures of which a renderer varies
 * four or five, so writing one out inline makes the second pipeline a copy of the first
 * with three lines changed. What is defaulted here is what every pipeline in this engine
 * has agreed on: a dynamic viewport and scissor so a resize costs no rebuild, one
 * sample, one colour attachment, and dynamic rendering rather than a render pass.
 *
 * The colour attachments are a list of 0..N formats, because how many there are is a
 * property of the pass rather than of the builder. A shadow pass draws depth and no
 * colour at all, which is colourFormats({}).
 *
 * The shader modules belong to the builder and are destroyed with it, since a module is
 * only needed while the pipeline is being compiled. The pipeline and its layout do not -
 * they are handed back for the caller to register with Resources, which is what destroys
 * them.
 *
 * A builder describes one pipeline. Building twice from one builder is allowed and gives
 * two pipelines that differ in nothing.
 **/
class Builder final {
 public:
    /**
     * @param device the device the pipeline is compiled for
     **/
    explicit Builder(const boost::shared_ptr<device::Device>& device);

    /**
     * Destroys the shader modules that were added.
     **/
    ~Builder();

    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;

    /**
     * What the pipeline is called, used only in the messages of the exceptions thrown
     * from here. Worth setting - a failed pipeline is otherwise anonymous.
     **/
    Builder& name(const std::string& name);

    /**
     * Add a stage from SPIR-V, which v3d_add_shader embeds as a uint32_t array.
     * @param stage which stage the module is for
     * @param code the SPIR-V words
     * @param bytes their size in bytes, not in words
     * @throw std::runtime_error if the module cannot be created
     **/
    Builder& shader(VkShaderStageFlagBits stage, const uint32_t* code, std::size_t bytes);

    /**
     * Add a vertex buffer binding the pipeline reads from.
     * @param binding which bound vertex buffer, matching the index a draw binds at
     * @param stride the size of one vertex
     **/
    Builder& vertexBinding(uint32_t binding, uint32_t stride, VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX);

    /**
     * Add one attribute read out of a binding.
     * @param location the location the vertex shader declares it at
     **/
    Builder& vertexAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset);

    /**
     * Defaults to a triangle list.
     **/
    Builder& topology(VkPrimitiveTopology topology);

    /**
     * Defaults to filled.
     **/
    Builder& polygon(VkPolygonMode mode);

    /**
     * Defaults to no culling with a counter clockwise front face - which is what 2D
     * wants, since a quad whose winding came out wrong should not silently vanish.
     **/
    Builder& cull(VkCullModeFlags mode, VkFrontFace face = VK_FRONT_FACE_COUNTER_CLOCKWISE);

    /**
     * Defaults to neither testing nor writing, which is what painter ordered 2D wants.
     * A pipeline that tests has to be built against a depth format as well.
     **/
    Builder& depth(bool test, bool write, VkCompareOp compare = VK_COMPARE_OP_LESS);

    /**
     * Whether the depth this pipeline writes is offset as it is written, which is what
     * separates a shadow map's own geometry from the surface tested against it.
     *
     * The bias itself is dynamic rather than built in: the constant and the slope factor
     * are a scene's numbers rather than a pipeline's, so a pipeline that asks for one is
     * told what it is by vkCmdSetDepthBias before it draws. Asking for none - the default -
     * leaves the dynamic state out, and a pipeline that never set one would be drawn with
     * whatever the last caller left behind.
     **/
    Builder& depthBias(bool enabled);

    /**
     * Defaults to straight alpha blending. Opaque geometry should turn it off - blending
     * costs bandwidth on every fragment whether or not any of them is transparent.
     **/
    Builder& blend(bool enabled);

    /**
     * How a blending pipeline combines what it draws with what is already there.
     *
     * The defaults are straight alpha over an opaque destination, which is what blend(true)
     * means and what everything presenting in this tree wants. A pipeline compositing into
     * something that is itself composited later wants a different destination alpha: a
     * factor of ZERO keeps the source's, where ONE_MINUS_SRC_ALPHA erodes it.
     *
     * The operation is VK_BLEND_OP_ADD either way. Nothing has wanted subtract or min, and
     * a caller that does is asking for a second thing rather than a different value of this
     * one.
     **/
    struct Blend {
        VkBlendFactor sourceColour{VK_BLEND_FACTOR_SRC_ALPHA};
        VkBlendFactor destinationColour{VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA};
        VkBlendFactor sourceAlpha{VK_BLEND_FACTOR_ONE};
        VkBlendFactor destinationAlpha{VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA};
    };

    /**
     * Blend with factors of the caller's own, which also turns blending on. Every colour
     * attachment gets them, since this is one answer for the pipeline the way blend() is.
     **/
    Builder& blend(const Blend& factors);

    /**
     * Add a descriptor set layout. They are numbered in the order they are added, so
     * set 0 - the per frame frequency of ADR-0008 - has to be added first.
     **/
    Builder& set(VkDescriptorSetLayout layout);

    /**
     * Declare the push constant block, which is where per object data lives per ADR-0008.
     * @param stages which stages read it
     * @param bytes its size, at most DrawItem::pushCapacity for anything a draw item carries
     **/
    Builder& push(VkShaderStageFlags stages, uint32_t bytes);

    /**
     * The format of the image the pass draws into. Dynamic rendering has no render pass
     * to take it from, so this is not optional.
     *
     * The same thing as colourFormats() with one format in it, which is what a pipeline
     * drawing into a single image wants to say.
     **/
    Builder& colourFormat(VkFormat format);

    /**
     * The formats of every image the pass draws into, in attachment order. An empty list
     * is a pipeline that writes no colour - a shadow pass - and each format gets the same
     * blend state, since blend() is one answer for the pipeline.
     **/
    Builder& colourFormats(const std::vector<VkFormat>& formats);

    /**
     * The format of the depth image, for a pipeline that tests or writes depth.
     **/
    Builder& depthFormat(VkFormat format);

    /**
     * Compile the pipeline.
     * @param cache the cache to compile against, so pipelines sharing state pay once
     * @return the pipeline and its layout, for the caller to hand to Resources
     * @throw std::runtime_error if the layout or the pipeline cannot be created
     **/
    Pipeline build(const boost::shared_ptr<Cache>& cache) const;

    /**
     * The three pieces of state a caller cannot otherwise check, each exactly as build()
     * will hand it to Vulkan - build() calls these rather than assembling its own, so what
     * is read here is what is compiled.
     *
     * **They exist because a VkPipeline cannot be read back.** Nothing about a compiled
     * pipeline says what it was built from, and a wrong answer in any of the three is a
     * picture rather than an error: a depth bias left out of the dynamic list silently
     * becomes the zero in the create info, so vkCmdSetDepthBias does nothing and a shadow
     * simply does not shift. Validation has nothing to say about any of that, so a consumer
     * that needs to know asks here.
     **/
    VkPipelineRasterizationStateCreateInfo rasterization() const;
    VkPipelineColorBlendAttachmentState colourBlend() const;
    std::vector<VkDynamicState> dynamics() const;

 private:
    boost::shared_ptr<device::Device> device_;
    std::string name_;

    std::vector<VkPipelineShaderStageCreateInfo> stages_;
    std::vector<VkShaderModule> modules_;
    std::vector<VkVertexInputBindingDescription> bindings_;
    std::vector<VkVertexInputAttributeDescription> attributes_;
    std::vector<VkDescriptorSetLayout> sets_;

    VkPrimitiveTopology topology_;
    VkPolygonMode polygon_;
    VkCullModeFlags cull_;
    VkFrontFace front_;
    bool depthTest_;
    bool depthWrite_;
    VkCompareOp depthCompare_;
    bool blend_;
    Blend factors_;
    bool depthBias_;
    VkShaderStageFlags pushStages_;
    uint32_t pushBytes_;
    std::vector<VkFormat> colours_;
    VkFormat depthFormat_;
};

};  // namespace v3d::render::realtime::vulkan::pipeline

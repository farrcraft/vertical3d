/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Device.h"
#include "PipelineCache.h"
#include "Resources.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan {

/**
 * Describes a graphics pipeline a chained call at a time, and builds it.
 *
 * A VkGraphicsPipelineCreateInfo is a dozen substructures of which a renderer varies
 * four or five, so writing one out inline makes the second pipeline a copy of the first
 * with three lines changed. What is defaulted here is what every pipeline in this engine
 * has agreed on: a dynamic viewport and scissor so a resize costs no rebuild, one
 * sample, one colour attachment, and dynamic rendering rather than a render pass.
 *
 * The shader modules belong to the builder and are destroyed with it, since a module is
 * only needed while the pipeline is being compiled. The pipeline and its layout do not -
 * they are handed back for the caller to register with Resources, which is what destroys
 * them.
 *
 * A builder describes one pipeline. Building twice from one builder is allowed and gives
 * two pipelines that differ in nothing.
 **/
class PipelineBuilder final {
 public:
    /**
     * @param device the device the pipeline is compiled for
     **/
    explicit PipelineBuilder(const boost::shared_ptr<Device>& device);

    /**
     * Destroys the shader modules that were added.
     **/
    ~PipelineBuilder();

    PipelineBuilder(const PipelineBuilder&) = delete;
    PipelineBuilder& operator=(const PipelineBuilder&) = delete;

    /**
     * What the pipeline is called, used only in the messages of the exceptions thrown
     * from here. Worth setting - a failed pipeline is otherwise anonymous.
     **/
    PipelineBuilder& name(const std::string& name);

    /**
     * Add a stage from SPIR-V, which v3d_add_shader embeds as a uint32_t array.
     * @param stage which stage the module is for
     * @param code the SPIR-V words
     * @param bytes their size in bytes, not in words
     * @throw std::runtime_error if the module cannot be created
     **/
    PipelineBuilder& shader(VkShaderStageFlagBits stage, const uint32_t* code, std::size_t bytes);

    /**
     * Add a vertex buffer binding the pipeline reads from.
     * @param binding which bound vertex buffer, matching the index a draw binds at
     * @param stride the size of one vertex
     **/
    PipelineBuilder& vertexBinding(uint32_t binding, uint32_t stride, VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX);

    /**
     * Add one attribute read out of a binding.
     * @param location the location the vertex shader declares it at
     **/
    PipelineBuilder& vertexAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset);

    /**
     * Defaults to a triangle list.
     **/
    PipelineBuilder& topology(VkPrimitiveTopology topology);

    /**
     * Defaults to filled.
     **/
    PipelineBuilder& polygon(VkPolygonMode mode);

    /**
     * Defaults to no culling with a counter clockwise front face - which is what 2D
     * wants, since a quad whose winding came out wrong should not silently vanish.
     **/
    PipelineBuilder& cull(VkCullModeFlags mode, VkFrontFace face = VK_FRONT_FACE_COUNTER_CLOCKWISE);

    /**
     * Defaults to neither testing nor writing, which is what painter ordered 2D wants.
     * A pipeline that tests has to be built against a depth format as well.
     **/
    PipelineBuilder& depth(bool test, bool write, VkCompareOp compare = VK_COMPARE_OP_LESS);

    /**
     * Defaults to straight alpha blending. Opaque geometry should turn it off - blending
     * costs bandwidth on every fragment whether or not any of them is transparent.
     **/
    PipelineBuilder& blend(bool enabled);

    /**
     * Add a descriptor set layout. They are numbered in the order they are added, so
     * set 0 - the per frame frequency of ADR-0008 - has to be added first.
     **/
    PipelineBuilder& set(VkDescriptorSetLayout layout);

    /**
     * Declare the push constant block, which is where per object data lives per ADR-0008.
     * @param stages which stages read it
     * @param bytes its size, at most DrawItem::pushCapacity for anything a draw item carries
     **/
    PipelineBuilder& push(VkShaderStageFlags stages, uint32_t bytes);

    /**
     * The format of the image the pass draws into. Dynamic rendering has no render pass
     * to take it from, so this is not optional.
     **/
    PipelineBuilder& colourFormat(VkFormat format);

    /**
     * The format of the depth image, for a pipeline that tests or writes depth.
     **/
    PipelineBuilder& depthFormat(VkFormat format);

    /**
     * Compile the pipeline.
     * @param cache the cache to compile against, so pipelines sharing state pay once
     * @return the pipeline and its layout, for the caller to hand to Resources
     * @throw std::runtime_error if the layout or the pipeline cannot be created
     **/
    Pipeline build(const boost::shared_ptr<PipelineCache>& cache) const;

 private:
    boost::shared_ptr<Device> device_;
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
    VkShaderStageFlags pushStages_;
    uint32_t pushBytes_;
    VkFormat colour_;
    VkFormat depthFormat_;
};

};  // namespace v3d::render::realtime::vulkan

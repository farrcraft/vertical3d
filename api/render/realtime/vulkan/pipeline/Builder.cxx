/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Builder.h"

#include <api/render/realtime/vulkan/device/Result.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace v3d::render::realtime::vulkan::pipeline {

/**
 **/
Builder::Builder(const boost::shared_ptr<device::Device>& device) :
    device_(device),
    name_("unnamed"),
    topology_(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
    polygon_(VK_POLYGON_MODE_FILL),
    cull_(VK_CULL_MODE_NONE),
    front_(VK_FRONT_FACE_COUNTER_CLOCKWISE),
    depthTest_(false),
    depthWrite_(false),
    depthCompare_(VK_COMPARE_OP_LESS),
    blend_(true),
    depthBias_(false),
    pushStages_(0),
    pushBytes_(0),
    colours_(1, VK_FORMAT_UNDEFINED),
    depthFormat_(VK_FORMAT_UNDEFINED) {
}

/**
 **/
Builder::~Builder() {
    for (VkShaderModule module : modules_) {
        vkDestroyShaderModule(device_->handle(), module, nullptr);
    }
    modules_.clear();
}

/**
 **/
Builder& Builder::name(const std::string& name) {
    name_ = name;
    return *this;
}

/**
 **/
Builder& Builder::shader(VkShaderStageFlagBits stage, const uint32_t* code, std::size_t bytes) {
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = bytes;
    info.pCode = code;

    VkShaderModule module = VK_NULL_HANDLE;
    VkResult result = vkCreateShaderModule(device_->handle(), &info, nullptr, &module);
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create a shader module for the " << name_ << " pipeline - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }
    modules_.push_back(module);

    VkPipelineShaderStageCreateInfo created{};
    created.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    created.stage = stage;
    created.module = module;
    created.pName = "main";
    stages_.push_back(created);

    return *this;
}

/**
 **/
Builder& Builder::vertexBinding(uint32_t binding, uint32_t stride, VkVertexInputRate rate) {
    VkVertexInputBindingDescription description{};
    description.binding = binding;
    description.stride = stride;
    description.inputRate = rate;
    bindings_.push_back(description);
    return *this;
}

/**
 **/
Builder& Builder::vertexAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset) {
    VkVertexInputAttributeDescription description{};
    description.location = location;
    description.binding = binding;
    description.format = format;
    description.offset = offset;
    attributes_.push_back(description);
    return *this;
}

/**
 **/
Builder& Builder::topology(VkPrimitiveTopology topology) {
    topology_ = topology;
    return *this;
}

/**
 **/
Builder& Builder::polygon(VkPolygonMode mode) {
    polygon_ = mode;
    return *this;
}

/**
 **/
Builder& Builder::cull(VkCullModeFlags mode, VkFrontFace face) {
    cull_ = mode;
    front_ = face;
    return *this;
}

/**
 **/
Builder& Builder::depth(bool test, bool write, VkCompareOp compare) {
    depthTest_ = test;
    depthWrite_ = write;
    depthCompare_ = compare;
    return *this;
}

/**
 **/
Builder& Builder::depthBias(bool enabled) {
    depthBias_ = enabled;
    return *this;
}

/**
 **/
Builder& Builder::blend(bool enabled) {
    blend_ = enabled;
    return *this;
}

/**
 **/
Builder& Builder::blend(const Blend& factors) {
    blend_ = true;
    factors_ = factors;
    return *this;
}

/**
 **/
Builder& Builder::set(VkDescriptorSetLayout layout) {
    sets_.push_back(layout);
    return *this;
}

/**
 **/
Builder& Builder::push(VkShaderStageFlags stages, uint32_t bytes) {
    pushStages_ = stages;
    pushBytes_ = bytes;
    return *this;
}

/**
 **/
Builder& Builder::colourFormat(VkFormat format) {
    colours_.assign(1, format);
    return *this;
}

/**
 **/
Builder& Builder::colourFormats(const std::vector<VkFormat>& formats) {
    colours_ = formats;
    return *this;
}

/**
 **/
Builder& Builder::depthFormat(VkFormat format) {
    depthFormat_ = format;
    return *this;
}

/**
 **/
VkPipelineRasterizationStateCreateInfo Builder::rasterization() const {
    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = polygon_;
    raster.cullMode = cull_;
    raster.frontFace = front_;
    raster.lineWidth = 1.0f;
    // the factors are dynamic, so this enables the bias without saying what it is
    raster.depthBiasEnable = depthBias_ ? VK_TRUE : VK_FALSE;
    return raster;
}

/**
 **/
VkPipelineColorBlendAttachmentState Builder::colourBlend() const {
    VkPipelineColorBlendAttachmentState attachment{};
    attachment.blendEnable = blend_ ? VK_TRUE : VK_FALSE;
    attachment.srcColorBlendFactor = factors_.sourceColour;
    attachment.dstColorBlendFactor = factors_.destinationColour;
    attachment.colorBlendOp = VK_BLEND_OP_ADD;
    attachment.srcAlphaBlendFactor = factors_.sourceAlpha;
    attachment.dstAlphaBlendFactor = factors_.destinationAlpha;
    attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    return attachment;
}

/**
 **/
std::vector<VkDynamicState> Builder::dynamics() const {
    // the viewport is dynamic so that a window resize costs no pipeline rebuild, and the
    // depth bias is dynamic for a pipeline that asked for one - its numbers belong to the
    // scene rather than to the pipeline
    std::vector<VkDynamicState> dynamics{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    if (depthBias_) {
        dynamics.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
    }
    return dynamics;
}

/**
 **/
Pipeline Builder::build(const boost::shared_ptr<Cache>& cache) const {
    if (stages_.empty()) {
        std::stringstream msg;
        msg << "The " << name_ << " pipeline was built with no shader stages";
        throw std::runtime_error(msg.str());
    }
    // an empty list is a pipeline that writes no colour, which a shadow pass is. An entry
    // left undefined is one nobody named, which is the default and is still a mistake
    for (const VkFormat format : colours_) {
        if (format == VK_FORMAT_UNDEFINED) {
            std::stringstream msg;
            msg << "The " << name_ << " pipeline was built with an undefined colour attachment format";
            throw std::runtime_error(msg.str());
        }
    }
    if ((depthTest_ || depthWrite_) && depthFormat_ == VK_FORMAT_UNDEFINED) {
        std::stringstream msg;
        msg << "The " << name_ << " pipeline tests or writes depth but was built with no depth format";
        throw std::runtime_error(msg.str());
    }

    VkDevice device = device_->handle();

    Pipeline built;
    built.pushStages = pushBytes_ > 0 ? pushStages_ : 0;

    VkPushConstantRange push{};
    push.stageFlags = pushStages_;
    push.offset = 0;
    push.size = pushBytes_;

    VkPipelineLayoutCreateInfo layout{};
    layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout.setLayoutCount = static_cast<uint32_t>(sets_.size());
    layout.pSetLayouts = sets_.empty() ? nullptr : sets_.data();
    layout.pushConstantRangeCount = built.pushStages != 0 ? 1 : 0;
    layout.pPushConstantRanges = built.pushStages != 0 ? &push : nullptr;

    VkResult result = vkCreatePipelineLayout(device, &layout, nullptr, &built.layout);
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create the " << name_ << " pipeline layout - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    VkPipelineVertexInputStateCreateInfo input{};
    input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    input.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings_.size());
    input.pVertexBindingDescriptions = bindings_.empty() ? nullptr : bindings_.data();
    input.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes_.size());
    input.pVertexAttributeDescriptions = attributes_.empty() ? nullptr : attributes_.data();

    VkPipelineInputAssemblyStateCreateInfo assembly{};
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = topology_;

    VkPipelineViewportStateCreateInfo viewport{};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;

    const VkPipelineRasterizationStateCreateInfo raster = rasterization();

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = depthTest_ ? VK_TRUE : VK_FALSE;
    depth.depthWriteEnable = depthWrite_ ? VK_TRUE : VK_FALSE;
    depth.depthCompareOp = depthCompare_;
    depth.maxDepthBounds = 1.0f;

    const VkPipelineColorBlendAttachmentState attachment = colourBlend();

    // blend() is one answer for the pipeline, so every attachment blends the same way
    const std::vector<VkPipelineColorBlendAttachmentState> attachments(colours_.size(), attachment);
    VkPipelineColorBlendStateCreateInfo blending{};
    blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blending.attachmentCount = static_cast<uint32_t>(attachments.size());
    blending.pAttachments = attachments.empty() ? nullptr : attachments.data();

    const std::vector<VkDynamicState> dynamics = this->dynamics();
    VkPipelineDynamicStateCreateInfo dynamic{};
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = static_cast<uint32_t>(dynamics.size());
    dynamic.pDynamicStates = dynamics.data();

    // dynamic rendering, so the formats come from here rather than from a render pass
    VkPipelineRenderingCreateInfo rendering{};
    rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering.colorAttachmentCount = static_cast<uint32_t>(colours_.size());
    rendering.pColorAttachmentFormats = colours_.empty() ? nullptr : colours_.data();
    rendering.depthAttachmentFormat = depthFormat_;

    VkGraphicsPipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.pNext = &rendering;
    info.stageCount = static_cast<uint32_t>(stages_.size());
    info.pStages = stages_.data();
    info.pVertexInputState = &input;
    info.pInputAssemblyState = &assembly;
    info.pViewportState = &viewport;
    info.pRasterizationState = &raster;
    info.pMultisampleState = &multisample;
    info.pDepthStencilState = &depth;
    info.pColorBlendState = &blending;
    info.pDynamicState = &dynamic;
    info.layout = built.layout;

    result = vkCreateGraphicsPipelines(device, cache ? cache->handle() : VK_NULL_HANDLE, 1, &info, nullptr, &built.pipeline);
    if (result != VK_SUCCESS) {
        vkDestroyPipelineLayout(device, built.layout, nullptr);
        std::stringstream msg;
        msg << "Unable to create the " << name_ << " pipeline - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    return built;
}

};  // namespace v3d::render::realtime::vulkan::pipeline

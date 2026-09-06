/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "PipelineBuilder.h"

#include <sstream>
#include <stdexcept>
#include <string>

#include "Result.h"

namespace v3d::render::realtime::vulkan {

/**
 **/
PipelineBuilder::PipelineBuilder(const boost::shared_ptr<Device>& device) :
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
    pushStages_(0),
    pushBytes_(0),
    colour_(VK_FORMAT_UNDEFINED),
    depthFormat_(VK_FORMAT_UNDEFINED) {
}

/**
 **/
PipelineBuilder::~PipelineBuilder() {
    for (VkShaderModule module : modules_) {
        vkDestroyShaderModule(device_->handle(), module, nullptr);
    }
    modules_.clear();
}

/**
 **/
PipelineBuilder& PipelineBuilder::name(const std::string& name) {
    name_ = name;
    return *this;
}

/**
 **/
PipelineBuilder& PipelineBuilder::shader(VkShaderStageFlagBits stage, const uint32_t* code, std::size_t bytes) {
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = bytes;
    info.pCode = code;

    VkShaderModule module = VK_NULL_HANDLE;
    VkResult result = vkCreateShaderModule(device_->handle(), &info, nullptr, &module);
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create a shader module for the " << name_ << " pipeline - " << resultString(result);
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
PipelineBuilder& PipelineBuilder::vertexBinding(uint32_t binding, uint32_t stride, VkVertexInputRate rate) {
    VkVertexInputBindingDescription description{};
    description.binding = binding;
    description.stride = stride;
    description.inputRate = rate;
    bindings_.push_back(description);
    return *this;
}

/**
 **/
PipelineBuilder& PipelineBuilder::vertexAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset) {
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
PipelineBuilder& PipelineBuilder::topology(VkPrimitiveTopology topology) {
    topology_ = topology;
    return *this;
}

/**
 **/
PipelineBuilder& PipelineBuilder::polygon(VkPolygonMode mode) {
    polygon_ = mode;
    return *this;
}

/**
 **/
PipelineBuilder& PipelineBuilder::cull(VkCullModeFlags mode, VkFrontFace face) {
    cull_ = mode;
    front_ = face;
    return *this;
}

/**
 **/
PipelineBuilder& PipelineBuilder::depth(bool test, bool write, VkCompareOp compare) {
    depthTest_ = test;
    depthWrite_ = write;
    depthCompare_ = compare;
    return *this;
}

/**
 **/
PipelineBuilder& PipelineBuilder::blend(bool enabled) {
    blend_ = enabled;
    return *this;
}

/**
 **/
PipelineBuilder& PipelineBuilder::set(VkDescriptorSetLayout layout) {
    sets_.push_back(layout);
    return *this;
}

/**
 **/
PipelineBuilder& PipelineBuilder::push(VkShaderStageFlags stages, uint32_t bytes) {
    pushStages_ = stages;
    pushBytes_ = bytes;
    return *this;
}

/**
 **/
PipelineBuilder& PipelineBuilder::colourFormat(VkFormat format) {
    colour_ = format;
    return *this;
}

/**
 **/
PipelineBuilder& PipelineBuilder::depthFormat(VkFormat format) {
    depthFormat_ = format;
    return *this;
}

/**
 **/
Pipeline PipelineBuilder::build(const boost::shared_ptr<PipelineCache>& cache) const {
    if (stages_.empty()) {
        std::stringstream msg;
        msg << "The " << name_ << " pipeline was built with no shader stages";
        throw std::runtime_error(msg.str());
    }
    if (colour_ == VK_FORMAT_UNDEFINED) {
        std::stringstream msg;
        msg << "The " << name_ << " pipeline was built with no colour attachment format";
        throw std::runtime_error(msg.str());
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
        msg << "Unable to create the " << name_ << " pipeline layout - " << resultString(result);
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

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = polygon_;
    raster.cullMode = cull_;
    raster.frontFace = front_;
    raster.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = depthTest_ ? VK_TRUE : VK_FALSE;
    depth.depthWriteEnable = depthWrite_ ? VK_TRUE : VK_FALSE;
    depth.depthCompareOp = depthCompare_;
    depth.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState attachment{};
    attachment.blendEnable = blend_ ? VK_TRUE : VK_FALSE;
    attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    attachment.colorBlendOp = VK_BLEND_OP_ADD;
    attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blending{};
    blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blending.attachmentCount = 1;
    blending.pAttachments = &attachment;

    // the viewport is dynamic so that a window resize costs no pipeline rebuild
    const VkDynamicState dynamics[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{};
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamics;

    // dynamic rendering, so the formats come from here rather than from a render pass
    VkPipelineRenderingCreateInfo rendering{};
    rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachmentFormats = &colour_;
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
        msg << "Unable to create the " << name_ << " pipeline - " << resultString(result);
        throw std::runtime_error(msg.str());
    }

    return built;
}

};  // namespace v3d::render::realtime::vulkan

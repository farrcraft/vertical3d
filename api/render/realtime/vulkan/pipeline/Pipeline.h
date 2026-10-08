/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <vector>

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
    bool scene;                     /**< whether the layout declares a set 2, where a pass binds its scene **/
    bool biased;                    /**< whether depth bias is dynamic state, set per pass **/
    bool writeDynamic;              /**< whether depth writing is dynamic state, which a pass may set **/
    bool depthWrite;                /**< whether it writes depth in a pass that does not say **/
    std::vector<VkFormat> colourFormats;  /**< what it was built to draw into, which the recorder checks against each pass's target **/
    VkFormat depthFormat;           /**< and its depth, or undefined for none **/
};

};  // namespace v3d::render::realtime::vulkan::pipeline

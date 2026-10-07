/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/Handle.h>

#include <vulkan/vulkan.h>

namespace v3d::render::realtime::vulkan::pipeline {

/**
 * What is bound at set 1 for a draw: per material data. A material outlives the frames that
 * draw with it, so the set is allocated once rather than per frame.
 **/
struct Material final {
    Material() noexcept;

    VkDescriptorSet set;
    TextureHandle texture;
};

};  // namespace v3d::render::realtime::vulkan::pipeline

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <vulkan/vulkan.h>

namespace v3d::render::realtime::vulkan::memory {

/**
 * What a buffer is bound as (vertex, index, uniform or transfer), and the size it starts at.
 **/
struct BufferSpec {
    VkBufferUsageFlags usage = 0;
    VkDeviceSize bytes = 0;
};

};  // namespace v3d::render::realtime::vulkan::memory

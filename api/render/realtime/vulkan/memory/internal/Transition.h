/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

namespace v3d::render::realtime::vulkan::memory::internal {

/** The layout an image leaves, and the layout it enters. **/
struct Transition {
    VkImageLayout from = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout to = VK_IMAGE_LAYOUT_UNDEFINED;
};

};  // namespace v3d::render::realtime::vulkan::memory::internal

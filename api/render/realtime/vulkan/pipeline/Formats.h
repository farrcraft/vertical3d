/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

namespace v3d::render::realtime::vulkan::pipeline {

/**
 * The formats of the attachments a pipeline draws into. A depth of VK_FORMAT_UNDEFINED is a
 * pipeline for a pass with no depth attachment.
 **/
struct Formats {
    VkFormat colour = VK_FORMAT_UNDEFINED;
    VkFormat depth = VK_FORMAT_UNDEFINED;
};

};  // namespace v3d::render::realtime::vulkan::pipeline

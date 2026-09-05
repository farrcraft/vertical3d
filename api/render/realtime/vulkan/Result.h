/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <string>

namespace v3d::render::realtime::vulkan {

/**
 * @param result a result code returned by any of the vulkan entry points
 * @return a readable form of the code, or its numeric value when it is not one we name
 **/
std::string resultString(VkResult result);

};  // namespace v3d::render::realtime::vulkan

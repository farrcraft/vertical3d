/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <stdexcept>
#include <string>

namespace v3d::render::realtime::vulkan::device {

/**
 * @param result a result code returned by any of the vulkan entry points
 * @return a readable form of the code, or its numeric value when it is not one we name
 **/
std::string resultString(VkResult result);

/**
 * What a failed call throws: what was being attempted, then the result in words.
 * @param what the attempt, as "Unable to ..."
 **/
std::runtime_error failure(VkResult result, const std::string& what);

/**
 * Throw failure() unless the call succeeded.
 * @param tolerated a second result that is not a failure - VK_INCOMPLETE, for a call that
 *        filled what it was given and had more
 **/
void check(VkResult result, const std::string& what, VkResult tolerated = VK_SUCCESS);

};  // namespace v3d::render::realtime::vulkan::device

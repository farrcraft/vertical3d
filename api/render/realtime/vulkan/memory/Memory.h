/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <optional>

namespace v3d::render::realtime::vulkan::memory {

/**
 * Pick a memory type that the allocation may use and that has the properties the caller
 * requires.
 *
 * Allocator's direct path chooses every memory type through this. The suballocated path
 * leaves the choice to the Vulkan Memory Allocator.
 *
 * @param device the physical device whose memory types are being chosen from
 * @param requirements what the device reported for the allocation, whose memoryTypeBits are
 *        the types it may use
 * @param properties what the memory has to be: host visible, device local, and so on
 * @return the index of a type satisfying both, or nothing when the device offers no such type
 **/
std::optional<uint32_t> memoryType(VkPhysicalDevice device, const VkMemoryRequirements& requirements, VkMemoryPropertyFlags properties);

};  // namespace v3d::render::realtime::vulkan::memory

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace v3d::render::realtime::vulkan::memory {

/**
 * Pick a memory type the device offers that both an allocation is allowed to use and the
 * caller can live with.
 *
 * Every allocation in the renderer goes through this rather than through a suballocating
 * allocator - there are two kinds of allocation, and each is made once at load time.
 *
 * @param device the physical device whose memory types are being chosen from
 * @param bits the memoryTypeBits a VkMemoryRequirements reported for the allocation
 * @param properties what the memory has to be - host visible, device local, and so on
 * @return the index of a type satisfying both
 * @throw std::runtime_error if the device offers no such type
 **/
uint32_t memoryType(VkPhysicalDevice device, uint32_t bits, VkMemoryPropertyFlags properties);

};  // namespace v3d::render::realtime::vulkan::memory

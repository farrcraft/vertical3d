/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Memory.h"

#include <optional>

namespace v3d::render::realtime::vulkan::memory {

/**
 **/
std::optional<uint32_t> memoryType(VkPhysicalDevice device, const VkMemoryRequirements& requirements, VkMemoryPropertyFlags properties) {
    const uint32_t bits = requirements.memoryTypeBits;
    VkPhysicalDeviceMemoryProperties available{};
    vkGetPhysicalDeviceMemoryProperties(device, &available);

    for (uint32_t index = 0; index < available.memoryTypeCount; index++) {
        const bool allowed = (bits & (1u << index)) != 0;
        const bool suitable = (available.memoryTypes[index].propertyFlags & properties) == properties;
        if (allowed && suitable) {
            return index;
        }
    }
    return std::nullopt;
}

};  // namespace v3d::render::realtime::vulkan::memory

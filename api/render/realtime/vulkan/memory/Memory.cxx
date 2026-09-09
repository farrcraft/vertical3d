/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Memory.h"

#include <sstream>
#include <stdexcept>

namespace v3d::render::realtime::vulkan::memory {

/**
 **/
uint32_t memoryType(VkPhysicalDevice device, uint32_t bits, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties available{};
    vkGetPhysicalDeviceMemoryProperties(device, &available);

    for (uint32_t index = 0; index < available.memoryTypeCount; index++) {
        const bool allowed = (bits & (1u << index)) != 0;
        const bool suitable = (available.memoryTypes[index].propertyFlags & properties) == properties;
        if (allowed && suitable) {
            return index;
        }
    }

    std::stringstream msg;
    msg << "No vulkan memory type is both allowed by the allocation and has the properties 0x" << std::hex << properties;
    throw std::runtime_error(msg.str());
}

};  // namespace v3d::render::realtime::vulkan::memory

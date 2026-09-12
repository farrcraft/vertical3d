/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Allocator.h"

#include "Memory.h"

namespace v3d::render::realtime::vulkan::memory {

namespace {

/**
 * Allocate for a resource whose requirements have already been read, and write what was
 * allocated. Buffers and images differ only in which call reports the requirements, so
 * everything after that is shared.
 **/
VkResult allocate(VkDevice device, VkPhysicalDevice physical, const VkMemoryRequirements& requirements,
    VkMemoryPropertyFlags properties, Allocation* allocation) {
    VkMemoryAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.allocationSize = requirements.size;
    info.memoryTypeIndex = memoryType(physical, requirements.memoryTypeBits, properties);

    VkDeviceMemory memory = VK_NULL_HANDLE;
    const VkResult result = vkAllocateMemory(device, &info, nullptr, &memory);
    if (result != VK_SUCCESS) {
        return result;
    }
    allocation->memory = memory;
    allocation->offset = 0;
    allocation->size = requirements.size;
    return VK_SUCCESS;
}

};  // namespace

/**
 **/
Allocation::Allocation() noexcept :
    memory(VK_NULL_HANDLE),
    offset(0),
    size(0) {
}

/**
 **/
bool Allocation::valid() const noexcept {
    return memory != VK_NULL_HANDLE;
}

/**
 **/
Allocator::Allocator(VkDevice device, VkPhysicalDevice physical) noexcept :
    device_(device),
    physical_(physical) {
}

/**
 **/
Allocator::~Allocator() {
    // the allocations belong to the resources that asked for them, and each frees its own
    // before the device it was allocated on goes away
}

/**
 **/
VkResult Allocator::bind(VkBuffer buffer, VkMemoryPropertyFlags properties, Allocation* allocation) {
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device_, buffer, &requirements);

    Allocation bound;
    const VkResult allocated = allocate(device_, physical_, requirements, properties, &bound);
    if (allocated != VK_SUCCESS) {
        return allocated;
    }
    const VkResult result = vkBindBufferMemory(device_, buffer, bound.memory, bound.offset);
    if (result != VK_SUCCESS) {
        // the caller has a buffer to destroy and no allocation to free, so this is given back
        // here rather than left for a caller that has nothing to name it with
        free(&bound);
        return result;
    }
    *allocation = bound;
    return VK_SUCCESS;
}

/**
 **/
VkResult Allocator::bind(VkImage image, VkMemoryPropertyFlags properties, Allocation* allocation) {
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device_, image, &requirements);

    Allocation bound;
    const VkResult allocated = allocate(device_, physical_, requirements, properties, &bound);
    if (allocated != VK_SUCCESS) {
        return allocated;
    }
    const VkResult result = vkBindImageMemory(device_, image, bound.memory, bound.offset);
    if (result != VK_SUCCESS) {
        free(&bound);
        return result;
    }
    *allocation = bound;
    return VK_SUCCESS;
}

/**
 **/
void Allocator::free(Allocation* allocation) noexcept {
    if (!allocation->valid()) {
        return;
    }
    vkFreeMemory(device_, allocation->memory, nullptr);
    *allocation = Allocation();
}

/**
 **/
VkResult Allocator::map(const Allocation& allocation, void** mapped) {
    return vkMapMemory(device_, allocation.memory, allocation.offset, allocation.size, 0, mapped);
}

/**
 **/
void Allocator::unmap(const Allocation& allocation) noexcept {
    vkUnmapMemory(device_, allocation.memory);
}

};  // namespace v3d::render::realtime::vulkan::memory

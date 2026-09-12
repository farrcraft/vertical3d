/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Allocator.h"

#pragma warning(push, 0)
#include <vk_mem_alloc.h>
#pragma warning(pop)

#include <sstream>
#include <stdexcept>

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
VkResult Allocator::suballocate(VkBuffer buffer, VkImage image, VkMemoryPropertyFlags properties, Allocation* allocation) {
    VmaAllocationCreateInfo request{};
    // the properties the caller asked for are required rather than preferred, so a device
    // that cannot offer them fails here the way the direct path's memoryType() does
    request.requiredFlags = properties;
    request.usage = VMA_MEMORY_USAGE_UNKNOWN;

    VmaAllocation handle = VK_NULL_HANDLE;
    VmaAllocationInfo info{};
    const VkResult result = buffer != VK_NULL_HANDLE
        ? vmaAllocateMemoryForBuffer(suballocator_, buffer, &request, &handle, &info)
        : vmaAllocateMemoryForImage(suballocator_, image, &request, &handle, &info);
    if (result != VK_SUCCESS) {
        return result;
    }

    const VkResult bound = buffer != VK_NULL_HANDLE
        ? vmaBindBufferMemory(suballocator_, handle, buffer)
        : vmaBindImageMemory(suballocator_, handle, image);
    if (bound != VK_SUCCESS) {
        vmaFreeMemory(suballocator_, handle);
        return bound;
    }

    allocation->memory = info.deviceMemory;
    allocation->offset = info.offset;
    allocation->size = info.size;
    allocation->handle = handle;
    return VK_SUCCESS;
}

/**
 **/
Allocation::Allocation() noexcept :
    memory(VK_NULL_HANDLE),
    offset(0),
    size(0),
    handle(VK_NULL_HANDLE) {
}

/**
 **/
bool Allocation::valid() const noexcept {
    return memory != VK_NULL_HANDLE;
}

/**
 **/
Allocator::Allocator(VkDevice device, VkPhysicalDevice physical, VkInstance instance, Kind kind) :
    device_(device),
    physical_(physical),
    kind_(kind),
    suballocator_(VK_NULL_HANDLE) {
    if (kind_ != Kind::Suballocated) {
        return;
    }
    VmaAllocatorCreateInfo info{};
    info.physicalDevice = physical_;
    info.device = device_;
    info.instance = instance;
    // the version the tree draws with - ADR-0001. A suballocator asked for a newer one than
    // the loader has calls entry points that are not there
    info.vulkanApiVersion = VK_API_VERSION_1_3;

    const VkResult result = vmaCreateAllocator(&info, &suballocator_);
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create the suballocating vulkan memory allocator - " << result;
        throw std::runtime_error(msg.str());
    }
}

/**
 **/
Allocator::Kind Allocator::kind() const noexcept {
    return kind_;
}

/**
 **/
Allocator::~Allocator() {
    // the allocations belong to the resources that asked for them, and each frees its own
    // before the device it was allocated on goes away
    if (suballocator_ != VK_NULL_HANDLE) {
        vmaDestroyAllocator(suballocator_);
        suballocator_ = VK_NULL_HANDLE;
    }
}

/**
 **/
VkResult Allocator::bind(VkBuffer buffer, VkMemoryPropertyFlags properties, Allocation* allocation) {
    if (kind_ == Kind::Suballocated) {
        return suballocate(buffer, VK_NULL_HANDLE, properties, allocation);
    }

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
    if (kind_ == Kind::Suballocated) {
        return suballocate(VK_NULL_HANDLE, image, properties, allocation);
    }

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
    if (allocation->handle != VK_NULL_HANDLE) {
        // the region is given back to the block it came from, which is not the same thing as
        // freeing the block - the suballocator decides when that happens
        vmaFreeMemory(suballocator_, allocation->handle);
    } else {
        vkFreeMemory(device_, allocation->memory, nullptr);
    }
    *allocation = Allocation();
}

/**
 **/
VkResult Allocator::map(const Allocation& allocation, void** mapped) {
    if (allocation.handle != VK_NULL_HANDLE) {
        // through the suballocator rather than the device: several regions can share one
        // block, and vkMapMemory on a block already mapped for another of them is invalid
        return vmaMapMemory(suballocator_, allocation.handle, mapped);
    }
    return vkMapMemory(device_, allocation.memory, allocation.offset, allocation.size, 0, mapped);
}

/**
 **/
void Allocator::unmap(const Allocation& allocation) noexcept {
    if (allocation.handle != VK_NULL_HANDLE) {
        vmaUnmapMemory(suballocator_, allocation.handle);
        return;
    }
    vkUnmapMemory(device_, allocation.memory);
}

};  // namespace v3d::render::realtime::vulkan::memory

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include "Allocation.h"

#include <cstdint>

// the Vulkan Memory Allocator's handle, declared here instead of included. vk_mem_alloc.h is
// large, and this header is included by most of the renderer. Repeating the library's typedef
// keeps vk_mem_alloc.h out of every file except Allocator.cxx and VmaImpl.cxx
VK_DEFINE_HANDLE(VmaAllocator)

namespace v3d::render::realtime::vulkan::memory {

/**
 * Gives a buffer or an image memory to live in, and takes it back.
 *
 * Kind::Suballocated, the default, places each resource in a region of a larger block, through
 * the Vulkan Memory Allocator. A device allocation is slow to make, and maxMemoryAllocationCount
 * is a real device limit, so a world that streams textures and meshes in needs it.
 * Kind::Direct makes one device allocation per resource, for a case that wants each resource's
 * memory to be its own.
 *
 * Every allocation in the renderer goes through Allocator, so the choice is made in one place.
 **/
class Allocator final {
 public:
    /**
     * How the memory behind a resource is found.
     **/
    enum class Kind : std::uint8_t {
        /**< one device allocation per resource **/
        Direct,
        /**< regions of larger blocks, through the Vulkan Memory Allocator **/
        Suballocated
    };

    /**
     * @param device the logical device allocations are made on
     * @param physical the physical device whose memory types are chosen from
     * @param instance the instance the device came from, which a suballocator needs to load
     *        the entry points it calls
     * @param kind how to find memory. Suballocated unless the consumer asked otherwise.
     * @throw std::runtime_error if a suballocator was asked for and could not be created
     **/
    Allocator(VkDevice device, VkPhysicalDevice physical, VkInstance instance, Kind kind = Kind::Suballocated);

    /**
     * @return how this finds memory
     **/
    Kind kind() const noexcept;

    ~Allocator();

    Allocator(const Allocator&) = delete;
    Allocator& operator=(const Allocator&) = delete;

    /**
     * Give a buffer memory and bind it there.
     *
     * @param buffer the buffer to allocate for, already created
     * @param properties what the memory has to be: host visible, device local, and so on
     * @param allocation where the allocation is written, untouched unless VK_SUCCESS
     * @return what the allocation or the bind returned. VK_ERROR_FEATURE_NOT_PRESENT means the
     *         device offers no memory type with those properties.
     **/
    VkResult bind(VkBuffer buffer, VkMemoryPropertyFlags properties, Allocation* allocation);

    /**
     * The same for an image.
     **/
    VkResult bind(VkImage image, VkMemoryPropertyFlags properties, Allocation* allocation);

    /**
     * Give an allocation back. Does nothing to one that names no memory, so a resource that
     * failed to allocate can free unconditionally.
     **/
    void free(Allocation* allocation) noexcept;

    /**
     * Map an allocation for the host to write through.
     *
     * @param allocation what to map, which has to have been allocated host visible
     * @param mapped where the pointer is written, untouched unless VK_SUCCESS
     * @return what the map returned
     **/
    VkResult map(const Allocation& allocation, void** mapped);

    /**
     **/
    void unmap(const Allocation& allocation) noexcept;

 private:
    /**
     * Allocate through the suballocator and bind, for whichever of the two is not null.
     * One function because the two differ only in which pair of vma calls they make.
     **/
    VkResult suballocate(VkBuffer buffer, VkImage image, VkMemoryPropertyFlags properties, Allocation* allocation);

    VkDevice device_;
    VkPhysicalDevice physical_;
    Kind kind_;
    VmaAllocator suballocator_;  /**< null unless kind_ is Suballocated **/
};

};  // namespace v3d::render::realtime::vulkan::memory

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace v3d::render::realtime::vulkan::memory {

/**
 * Memory a resource lives in, and the handle whoever allocated it needs to give it back.
 *
 * A resource holds one of these rather than a VkDeviceMemory, because what identifies an
 * allocation is the allocator's business: a suballocator hands out a region of a larger
 * block, so the device memory alone does not say which allocation was meant.
 **/
struct Allocation final {
    Allocation() noexcept;

    /**
     * @return whether this names memory at all
     **/
    bool valid() const noexcept;

    VkDeviceMemory memory;  /**< the block the resource lives in **/
    VkDeviceSize offset;    /**< where in that block it starts **/
    VkDeviceSize size;      /**< how much of it the resource was given **/
};

/**
 * Gives a buffer or an image memory to live in, and takes it back.
 *
 * One allocation per resource, made from the device directly. That is the right shape for
 * the resources this tree creates, which are made once at load time and freed at shutdown;
 * it is the wrong one for an application with per-frame buffers, because
 * maxMemoryAllocationCount is a real device limit and a suballocator is what an allocation
 * per frame per resource needs.
 *
 * Every allocation in the renderer goes through here so that there is one place for such an
 * allocator to be, rather than five sites each calling vkAllocateMemory for themselves.
 **/
class Allocator final {
 public:
    /**
     * @param device the logical device allocations are made on
     * @param physical the physical device whose memory types are chosen from
     **/
    Allocator(VkDevice device, VkPhysicalDevice physical) noexcept;

    ~Allocator();

    Allocator(const Allocator&) = delete;
    Allocator& operator=(const Allocator&) = delete;

    /**
     * Give a buffer memory and bind it there.
     *
     * @param buffer the buffer to allocate for, already created
     * @param properties what the memory has to be - host visible, device local, and so on
     * @param allocation where the allocation is written, untouched unless VK_SUCCESS
     * @return what the allocation or the bind returned
     * @throw std::runtime_error if the device offers no memory type with those properties,
     *        which is a device that cannot do this rather than an allocation that failed
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
    VkDevice device_;
    VkPhysicalDevice physical_;
};

};  // namespace v3d::render::realtime::vulkan::memory

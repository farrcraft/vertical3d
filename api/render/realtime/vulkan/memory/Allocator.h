/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

// the allocator's own handles, declared rather than included: vk_mem_alloc.h is large and
// this header is reached from most of the renderer. Repeating the library's own typedef is
// what keeps that header in the one translation unit that implements it - VmaImpl.cxx
VK_DEFINE_HANDLE(VmaAllocator)
VK_DEFINE_HANDLE(VmaAllocation)

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
    /**< what the suballocator calls this region, and null when nothing suballocated it **/
    VmaAllocation handle;
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
     * How the memory behind a resource is found.
     **/
    enum class Kind {
        /**< one device allocation per resource, which is every app in this tree **/
        Direct,
        /**< regions of larger blocks, through the Vulkan Memory Allocator **/
        Suballocated
    };

    /**
     * @param device the logical device allocations are made on
     * @param physical the physical device whose memory types are chosen from
     * @param instance the instance the device came from, which a suballocator needs to load
     *        the entry points it calls
     * @param kind how to find memory - Direct unless the consumer asked otherwise
     * @throw std::runtime_error if a suballocator was asked for and could not be created
     **/
    Allocator(VkDevice device, VkPhysicalDevice physical, VkInstance instance, Kind kind = Kind::Direct);

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

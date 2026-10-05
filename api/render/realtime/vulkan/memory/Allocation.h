/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

// the allocator's own handle, declared rather than included: vk_mem_alloc.h is large and
// this header is included by most of the renderer. Repeating the library's own typedef keeps
// that header in the one translation unit that implements it - VmaImpl.cxx
VK_DEFINE_HANDLE(VmaAllocation)

namespace v3d::render::realtime::vulkan::memory {

/**
 * Memory a resource lives in, and the handle whoever allocated it needs to give it back.
 *
 * A resource holds one of these rather than a VkDeviceMemory, because what identifies an
 * allocation depends on the allocator: a suballocator hands out a region of a larger
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

};  // namespace v3d::render::realtime::vulkan::memory

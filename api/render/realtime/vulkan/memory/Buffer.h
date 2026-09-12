/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/memory/Allocator.h>

#include <vulkan/vulkan.h>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::memory {

/**
 * A host visible buffer the cpu reaches straight into, kept mapped for its whole life.
 *
 * This is the buffer a frame's geometry is built in: a batcher rewrites the whole thing
 * every frame, so a staging copy to device local memory would cost more than the slower
 * reads do. Static geometry wants the opposite trade and is not what this is for. It is
 * also what a transfer the cpu has to see lands in, which is the other direction of the
 * same trade.
 *
 * The allocation is coherent, so a write is visible to the device without a flush and a
 * completed transfer is visible to the cpu without an invalidate.
 **/
class Buffer final {
 public:
    /**
     * @param device the device to allocate on
     * @param usage what the buffer will be bound as - vertex, index or transfer source
     * @param bytes the size to start at, which grow() raises as the content demands
     **/
    Buffer(const boost::shared_ptr<device::Device>& device, VkBufferUsageFlags usage, VkDeviceSize bytes);

    /**
     **/
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    /**
     * @return the underlying buffer handle, which a call to grow() invalidates
     **/
    VkBuffer handle() const noexcept;

    /**
     * @return how many bytes the buffer holds
     **/
    VkDeviceSize size() const noexcept;

    /**
     * Make the buffer at least this large, doubling until it is.
     *
     * Growing replaces the allocation, so this waits for the device to finish with the
     * old one. A buffer only grows in the first few frames after a size changes.
     *
     * @return whether the buffer was reallocated, which invalidates handle()
     **/
    bool grow(VkDeviceSize bytes);

    /**
     * Copy into the mapped allocation.
     * @throw std::runtime_error if the write would run off the end
     **/
    void write(const void* data, VkDeviceSize bytes, VkDeviceSize offset = 0);

    /**
     * Copy out of the mapped allocation.
     *
     * The allocation is host visible and coherent rather than cached, so reading it back is
     * far slower than writing it. That is the right trade for a buffer the device fills
     * once and the cpu reads once, and the wrong one for anything doing it per frame.
     *
     * @throw std::runtime_error if the read would run off the end
     **/
    void read(void* data, VkDeviceSize bytes, VkDeviceSize offset = 0) const;

 private:
    /**
     **/
    void create(VkDeviceSize bytes);

    /**
     **/
    void destroy();

    boost::shared_ptr<device::Device> device_;
    VkBufferUsageFlags usage_;
    VkBuffer buffer_;
    Allocation memory_;
    VkDeviceSize size_;
    void* mapped_;
};

};  // namespace v3d::render::realtime::vulkan::memory

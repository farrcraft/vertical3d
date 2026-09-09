/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>

#include <vulkan/vulkan.h>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::memory {

/**
 * A host visible buffer the cpu writes straight into, kept mapped for its whole life.
 *
 * This is the buffer a frame's geometry is built in: a batcher rewrites the whole thing
 * every frame, so a staging copy to device local memory would cost more than the slower
 * reads do. Static geometry wants the opposite trade and is not what this is for.
 *
 * The allocation is coherent, so a write is visible to the device without a flush.
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
    VkDeviceMemory memory_;
    VkDeviceSize size_;
    void* mapped_;
};

};  // namespace v3d::render::realtime::vulkan::memory

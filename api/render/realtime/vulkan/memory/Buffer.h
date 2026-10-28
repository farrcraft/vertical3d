/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>

#include <vulkan/vulkan.h>

#include "Allocation.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::memory {

/**
 * What a buffer is bound as (vertex, index, uniform or transfer), and the size it starts at.
 **/
struct BufferSpec {
    VkBufferUsageFlags usage = 0;
    VkDeviceSize bytes = 0;
};

/**
 * A host visible buffer the CPU writes directly, kept mapped for its whole life.
 *
 * A frame's geometry is built in this buffer. A batcher rewrites all of it every frame, so a
 * staging copy to device local memory would cost more than the slower reads do. Static
 * geometry should use DeviceBuffer instead. A transfer that the CPU reads back also lands in
 * a Buffer.
 *
 * The allocation is coherent, so a write is visible to the device without a flush and a
 * completed transfer is visible to the CPU without an invalidate.
 **/
class Buffer final {
 public:
    /**
     * @param device the device to allocate on
     * @param spec what the buffer is bound as, and its size, which never changes
     **/
    Buffer(const boost::shared_ptr<device::Device>& device, const BufferSpec& spec);

    /**
     **/
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    /**
     * @return the underlying buffer handle
     **/
    VkBuffer handle() const noexcept;

    /**
     * @return how many bytes the buffer holds
     **/
    VkDeviceSize size() const noexcept;

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
     * once and the CPU reads once, and the wrong one for anything doing it per frame.
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

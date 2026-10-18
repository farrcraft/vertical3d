/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>

#include <vulkan/vulkan.h>

#include "Allocation.h"
#include "Buffer.h"
#include "Uploader.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::memory {

/**
 * A device local buffer, filled through a staging copy.
 *
 * Geometry built once and drawn for the life of the process pays for one staging copy at load
 * time. Every frame after that reads it from the memory closest to the device. A batcher that
 * rewrites its whole content every frame should use Buffer instead.
 *
 * The buffer is not mapped and cannot be written to directly. upload() is the only way to
 * fill it. upload() waits for the copy and not for frames in flight, so no frame may be
 * reading the buffer when it is called. The constructors upload into a buffer nothing has
 * read yet.
 **/
class DeviceBuffer final {
 public:
    /**
     * @param device the device to allocate on
     * @param uploader runs the staging copy
     * @param usage what the buffer will be bound as. Transfer destination usage is added.
     * @param bytes how large to allocate, which upload() never changes
     * @throw std::runtime_error if the buffer cannot be created or allocated
     **/
    DeviceBuffer(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Uploader>& uploader,
        VkBufferUsageFlags usage, VkDeviceSize bytes);

    /**
     * Allocate and fill in one step, as static geometry needs.
     * @param data the content to copy in, of bytes length
     **/
    DeviceBuffer(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Uploader>& uploader,
        VkBufferUsageFlags usage, const void* data, VkDeviceSize bytes);

    /**
     **/
    ~DeviceBuffer();

    DeviceBuffer(const DeviceBuffer&) = delete;
    DeviceBuffer& operator=(const DeviceBuffer&) = delete;

    /**
     * @return the underlying buffer handle, stable for the life of the buffer
     **/
    VkBuffer handle() const noexcept;

    /**
     * @return how many bytes the buffer holds
     **/
    VkDeviceSize size() const noexcept;

    /**
     * Copy into device local memory through a staging buffer, waiting for the copy.
     * @throw std::runtime_error if the write would run off the end
     **/
    void upload(const void* data, VkDeviceSize bytes, VkDeviceSize offset = 0);

 private:
    /**
     **/
    void create(const BufferSpec& spec);

    /**
     **/
    void destroy();

    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<Uploader> uploader_;
    VkBuffer buffer_;
    Allocation memory_;
    VkDeviceSize size_;
};

};  // namespace v3d::render::realtime::vulkan::memory

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>

#include <vulkan/vulkan.h>

#include "Uploader.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::memory {

/**
 * A device local buffer, filled through a staging copy.
 *
 * This is the other half of the trade Buffer describes: geometry that is built once and
 * drawn for the life of the process pays for one staging copy at load time and is read
 * out of the memory closest to the device for every frame after it. A batcher rewriting
 * its whole content every frame wants Buffer instead.
 *
 * The buffer is not mapped and cannot be written to directly - upload() is the only way
 * anything reaches it, and it waits for the copy, so nothing may be in flight against
 * the buffer when it is called.
 **/
class DeviceBuffer final {
 public:
    /**
     * @param device the device to allocate on
     * @param uploader runs the staging copy
     * @param usage what the buffer will be bound as - transfer destination is added
     * @param bytes how large to allocate, which upload() never changes
     * @throw std::runtime_error if the buffer cannot be created or allocated
     **/
    DeviceBuffer(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Uploader>& uploader,
        VkBufferUsageFlags usage, VkDeviceSize bytes);

    /**
     * Allocate and fill in one step, which is what static geometry wants.
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
    void create(VkBufferUsageFlags usage, VkDeviceSize bytes);

    /**
     **/
    void destroy();

    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<Uploader> uploader_;
    VkBuffer buffer_;
    VkDeviceMemory memory_;
    VkDeviceSize size_;
};

};  // namespace v3d::render::realtime::vulkan::memory

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "DeviceBuffer.h"

#include <api/render/realtime/vulkan/device/Result.h>

#include <sstream>
#include <stdexcept>

#include "Buffer.h"
#include "Memory.h"

namespace v3d::render::realtime::vulkan::memory {

/**
 **/
DeviceBuffer::DeviceBuffer(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Uploader>& uploader,
    VkBufferUsageFlags usage, VkDeviceSize bytes) :
    device_(device),
    uploader_(uploader),
    buffer_(VK_NULL_HANDLE),
    size_(0) {
    if (!uploader_) {
        throw std::runtime_error("A device local buffer needs an uploader to be filled through");
    }
    create(usage, bytes > 0 ? bytes : 1);
}

/**
 **/
DeviceBuffer::DeviceBuffer(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Uploader>& uploader,
    VkBufferUsageFlags usage, const void* data, VkDeviceSize bytes) :
    DeviceBuffer(device, uploader, usage, bytes) {
    upload(data, bytes);
}

/**
 **/
DeviceBuffer::~DeviceBuffer() {
    destroy();
}

/**
 **/
void DeviceBuffer::create(VkBufferUsageFlags usage, VkDeviceSize bytes) {
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = bytes;
    // nothing reaches device local memory except through a copy, so every buffer here is
    // a transfer destination whatever else the caller asked for
    info.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device_->handle(), &info, nullptr, &buffer_);
    if (result != VK_SUCCESS) {
        buffer_ = VK_NULL_HANDLE;
        std::stringstream msg;
        msg << "Unable to create a device local vulkan buffer - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    result = device_->allocator().bind(buffer_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memory_);
    if (result != VK_SUCCESS) {
        destroy();
        std::stringstream msg;
        msg << "Unable to allocate device local memory for a vulkan buffer - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    size_ = bytes;
}

/**
 **/
void DeviceBuffer::destroy() {
    if (buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_->handle(), buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
    }
    device_->allocator().free(&memory_);
    size_ = 0;
}

/**
 **/
VkBuffer DeviceBuffer::handle() const noexcept {
    return buffer_;
}

/**
 **/
VkDeviceSize DeviceBuffer::size() const noexcept {
    return size_;
}

/**
 **/
void DeviceBuffer::upload(const void* data, VkDeviceSize bytes, VkDeviceSize offset) {
    if (data == nullptr || bytes == 0) {
        return;
    }
    if (offset + bytes > size_) {
        throw std::runtime_error("An upload ran off the end of a device local vulkan buffer");
    }

    Buffer staging(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, bytes);
    staging.write(data, bytes);

    VkBuffer source = staging.handle();
    VkBuffer destination = buffer_;
    uploader_->oneShot([source, destination, bytes, offset](VkCommandBuffer commands) {
        VkBufferCopy copy{};
        copy.srcOffset = 0;
        copy.dstOffset = offset;
        copy.size = bytes;
        vkCmdCopyBuffer(commands, source, destination, 1, &copy);

        // waiting on the queue orders the copy against what comes after it but does not
        // make its writes visible to a later read - the barrier is what does that
        VkBufferMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = destination;
        barrier.offset = offset;
        barrier.size = bytes;

        VkDependencyInfo dependency{};
        dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency.bufferMemoryBarrierCount = 1;
        dependency.pBufferMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(commands, &dependency);
    });
    // the copy has run by the time oneShot returns, so the staging buffer can go
}

};  // namespace v3d::render::realtime::vulkan::memory

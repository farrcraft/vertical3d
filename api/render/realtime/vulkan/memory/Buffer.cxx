/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Buffer.h"

#include <api/render/realtime/vulkan/device/Result.h>

#include "Memory.h"

#include <cstring>
#include <sstream>
#include <stdexcept>

namespace v3d::render::realtime::vulkan::memory {

/**
 **/
Buffer::Buffer(const boost::shared_ptr<device::Device>& device, VkBufferUsageFlags usage, VkDeviceSize bytes) :
    device_(device),
    usage_(usage),
    buffer_(VK_NULL_HANDLE),
    memory_(VK_NULL_HANDLE),
    size_(0),
    mapped_(nullptr) {
    create(bytes > 0 ? bytes : 1);
}

/**
 **/
Buffer::~Buffer() {
    destroy();
}

/**
 **/
void Buffer::create(VkDeviceSize bytes) {
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = bytes;
    info.usage = usage_;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device_->handle(), &info, nullptr, &buffer_);
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create a vulkan buffer - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device_->handle(), buffer_, &requirements);

    VkMemoryAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memoryType(device_->physical(), requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    result = vkAllocateMemory(device_->handle(), &allocation, nullptr, &memory_);
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(device_->handle(), buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
        std::stringstream msg;
        msg << "Unable to allocate memory for a vulkan buffer - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    result = vkBindBufferMemory(device_->handle(), buffer_, memory_, 0);
    if (result != VK_SUCCESS) {
        destroy();
        std::stringstream msg;
        msg << "Unable to bind memory to a vulkan buffer - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    result = vkMapMemory(device_->handle(), memory_, 0, bytes, 0, &mapped_);
    if (result != VK_SUCCESS) {
        destroy();
        std::stringstream msg;
        msg << "Unable to map a vulkan buffer - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    size_ = bytes;
}

/**
 **/
void Buffer::destroy() {
    if (mapped_ != nullptr) {
        vkUnmapMemory(device_->handle(), memory_);
        mapped_ = nullptr;
    }
    if (buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_->handle(), buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
    }
    if (memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_->handle(), memory_, nullptr);
        memory_ = VK_NULL_HANDLE;
    }
    size_ = 0;
}

/**
 **/
VkBuffer Buffer::handle() const noexcept {
    return buffer_;
}

/**
 **/
VkDeviceSize Buffer::size() const noexcept {
    return size_;
}

/**
 **/
bool Buffer::grow(VkDeviceSize bytes) {
    if (bytes <= size_) {
        return false;
    }

    VkDeviceSize target = size_;
    while (target < bytes) {
        target *= 2;
    }

    // the allocation the device may still be reading out of is about to go away
    vkDeviceWaitIdle(device_->handle());
    destroy();
    create(target);
    return true;
}

/**
 **/
void Buffer::write(const void* data, VkDeviceSize bytes, VkDeviceSize offset) {
    if (bytes == 0) {
        return;
    }
    if (offset + bytes > size_) {
        throw std::runtime_error("A write ran off the end of a vulkan buffer");
    }
    std::memcpy(static_cast<unsigned char*>(mapped_) + offset, data, static_cast<std::size_t>(bytes));
}

};  // namespace v3d::render::realtime::vulkan::memory

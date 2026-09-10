/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CommandPool.h"

#include <api/render/realtime/vulkan/device/Result.h>

#include <sstream>
#include <stdexcept>
#include <vector>

namespace v3d::render::realtime::vulkan::frame {

/**
 **/
CommandPool::CommandPool(const boost::shared_ptr<device::Device>& device, uint32_t family) :
    device_(device),
    pool_(VK_NULL_HANDLE) {
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = family;

    VkResult result = vkCreateCommandPool(device_->handle(), &createInfo, nullptr, &pool_);
    if (result != VK_SUCCESS) {
        pool_ = VK_NULL_HANDLE;
        std::stringstream msg;
        msg << "Unable to create a vulkan command pool - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }
}

/**
 **/
CommandPool::~CommandPool() {
    if (pool_ != VK_NULL_HANDLE) {
        // the buffers allocated from the pool go with it
        vkDestroyCommandPool(device_->handle(), pool_, nullptr);
        pool_ = VK_NULL_HANDLE;
    }
}

/**
 **/
VkCommandPool CommandPool::handle() const noexcept {
    return pool_;
}

/**
 **/
std::vector<VkCommandBuffer> CommandPool::allocate(uint32_t count) const {
    std::vector<VkCommandBuffer> buffers(count, VK_NULL_HANDLE);
    if (count == 0) {
        return buffers;
    }

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = pool_;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = count;

    VkResult result = vkAllocateCommandBuffers(device_->handle(), &allocateInfo, buffers.data());
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to allocate vulkan command buffers - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    return buffers;
}

};  // namespace v3d::render::realtime::vulkan::frame

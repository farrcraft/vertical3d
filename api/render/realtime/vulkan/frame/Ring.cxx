/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Ring.h"

#include <api/render/realtime/vulkan/device/Result.h>

#include <sstream>
#include <stdexcept>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 **/
Ring::Ring(const boost::shared_ptr<device::Device>& device, uint32_t framesInFlight) :
    device_(device),
    framesInFlight_(framesInFlight > 0 ? framesInFlight : 1),
    frame_(0) {
    pool_ = boost::make_shared<CommandPool>(device_, device_->families().graphics);
    commands_ = pool_->allocate(framesInFlight_);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // signalled, so the first frame does not wait on a submission that never happened
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t index = 0; index < framesInFlight_; index++) {
        VkFence fence = VK_NULL_HANDLE;
        VkResult result = vkCreateFence(device_->handle(), &fenceInfo, nullptr, &fence);
        if (result != VK_SUCCESS) {
            std::stringstream msg;
            msg << "Unable to create a vulkan fence - " << device::resultString(result);
            throw std::runtime_error(msg.str());
        }
        inFlight_.push_back(fence);
    }
}

/**
 **/
Ring::~Ring() {
    // nothing may be waiting on a fence when it is destroyed
    waitIdle();

    for (VkFence fence : inFlight_) {
        vkDestroyFence(device_->handle(), fence, nullptr);
    }
    inFlight_.clear();
}

/**
 **/
boost::shared_ptr<device::Device> Ring::device() const noexcept {
    return device_;
}

/**
 **/
uint32_t Ring::framesInFlight() const noexcept {
    return framesInFlight_;
}

/**
 **/
uint32_t Ring::frame() const noexcept {
    return frame_;
}

/**
 **/
void Ring::waitFrame() const {
    VkFence fence = inFlight_[frame_];
    VkResult result = vkWaitForFences(device_->handle(), 1, &fence, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to wait on a vulkan frame fence - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }
}

/**
 **/
void Ring::waitIdle() const {
    vkDeviceWaitIdle(device_->handle());
}

/**
 **/
VkFence Ring::fence() const noexcept {
    return inFlight_[frame_];
}

/**
 **/
VkCommandBuffer Ring::begin() {
    waitFrame();

    VkFence fence = inFlight_[frame_];
    VkResult result = vkResetFences(device_->handle(), 1, &fence);
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to reset a vulkan frame fence - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    VkCommandBuffer commands = commands_[frame_];
    result = vkResetCommandBuffer(commands, 0);
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to reset a vulkan command buffer - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(commands, &beginInfo);
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to begin a vulkan command buffer - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    return commands;
}

/**
 **/
void Ring::advance() noexcept {
    frame_ = (frame_ + 1) % framesInFlight_;
}

};  // namespace v3d::render::realtime::vulkan::frame

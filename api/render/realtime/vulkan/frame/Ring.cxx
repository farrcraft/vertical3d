/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Ring.h"

#include <api/render/realtime/vulkan/device/Result.h>

#include <utility>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 **/
Ring::Ring(const boost::shared_ptr<device::Device>& device, uint32_t framesInFlight) :
    device_(device),
    framesInFlight_(framesInFlight > 0 ? framesInFlight : 1),
    frame_(0),
    begun_(0),
    retired_(framesInFlight_) {
    pool_ = boost::make_shared<CommandPool>(device_, device_->families().graphics);
    commands_ = pool_->allocate(framesInFlight_);
    timings_ = boost::make_shared<Timings>(device_, framesInFlight_);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // signalled, so the first frame does not wait on a submission that never happened
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t index = 0; index < framesInFlight_; index++) {
        VkFence fence = VK_NULL_HANDLE;
        VkResult result = vkCreateFence(device_->handle(), &fenceInfo, nullptr, &fence);
        if (result != VK_SUCCESS) {
            // the destructor does not run for a constructor that throws, so the fences
            // already made are destroyed here
            for (VkFence made : inFlight_) {
                vkDestroyFence(device_->handle(), made, nullptr);
            }
            inFlight_.clear();
        }
        device::check(result, "Unable to create a vulkan fence");
        inFlight_.push_back(fence);
    }
}

/**
 **/
Ring::~Ring() {
    // nothing may be waiting on a fence when it is destroyed, and nothing retired may still
    // be read by a frame
    waitIdle();
    retired_.flush();

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
    device::check(result, "Unable to wait on a vulkan frame fence");
}

/**
 **/
void Ring::waitIdle() const {
    vkDeviceWaitIdle(device_->handle());
}

/**
 **/
uint64_t Ring::begun() const noexcept {
    return begun_;
}

/**
 **/
void Ring::skip() noexcept {
    skipped_++;
}

/**
 **/
uint64_t Ring::turns() const noexcept {
    return begun_ + skipped_;
}

/**
 **/
void Ring::retire(std::function<void()> destroy) {
    retired_.retire(begun_, std::move(destroy));
}

/**
 **/
VkFence Ring::submitting() {
    VkFence fence = inFlight_[frame_];
    const VkResult result = vkResetFences(device_->handle(), 1, &fence);
    device::check(result, "Unable to reset a vulkan frame fence");
    return fence;
}

/**
 **/
VkCommandBuffer Ring::begin() {
    waitFrame();

    VkCommandBuffer commands = commands_[frame_];
    VkResult result = vkResetCommandBuffer(commands, 0);
    device::check(result, "Unable to reset a vulkan command buffer");

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(commands, &beginInfo);
    device::check(result, "Unable to begin a vulkan command buffer");

    // what this slot timed the last time it was used is readable now its fence has signalled
    timings_->begin(commands, frame_);

    // counted only once nothing can throw, because a begin that failed waited on a slot without
    // moving past it, and counting it would collect a frame early
    begun_++;
    retired_.collect(begun_);

    return commands;
}

/**
 **/
void Ring::advance() noexcept {
    frame_ = (frame_ + 1) % framesInFlight_;
}

/**
 **/
Timings& Ring::timings() noexcept {
    return *timings_;
}

};  // namespace v3d::render::realtime::vulkan::frame

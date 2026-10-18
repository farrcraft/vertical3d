/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Uploader.h"

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/device/Result.h>
#include <api/render/realtime/vulkan/frame/CommandPool.h>

#include <cstdint>
#include <functional>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::memory {

/**
 **/
Uploader::Uploader(const boost::shared_ptr<device::Device>& device) :
    device_(device),
    commands_(VK_NULL_HANDLE),
    done_(VK_NULL_HANDLE) {
    pool_ = boost::make_shared<frame::CommandPool>(device_, device_->families().graphics);
    std::vector<VkCommandBuffer> buffers = pool_->allocate(1);
    commands_ = buffers.front();

    // made last, so that nothing after it can throw and leave it to a destructor that never runs
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    device::check(vkCreateFence(device_->handle(), &fenceInfo, nullptr, &done_), "Unable to create the vulkan upload fence");
}

/**
 **/
Uploader::~Uploader() {
    // the buffer is freed with the pool
    vkDestroyFence(device_->handle(), done_, nullptr);
}

/**
 **/
void Uploader::oneShot(const std::function<void(VkCommandBuffer)>& record) const {
    if (!record) {
        return;
    }

    VkResult result = vkResetFences(device_->handle(), 1, &done_);
    device::check(result, "Unable to reset the vulkan upload fence");

    result = vkResetCommandBuffer(commands_, 0);
    device::check(result, "Unable to reset the vulkan upload command buffer");

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(commands_, &begin);
    device::check(result, "Unable to begin the vulkan upload command buffer");

    record(commands_);

    result = vkEndCommandBuffer(commands_);
    device::check(result, "Unable to end the vulkan upload command buffer");

    VkCommandBufferSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    info.commandBuffer = commands_;

    VkSubmitInfo2 submission{};
    submission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submission.commandBufferInfoCount = 1;
    submission.pCommandBufferInfos = &info;

    result = vkQueueSubmit2(device_->graphicsQueue(), 1, &submission, done_);
    device::check(result, "Unable to submit a vulkan upload");

    // the caller's staging allocation goes away when this returns. Only this submission is
    // waited for, and frames in flight go on running
    result = vkWaitForFences(device_->handle(), 1, &done_, VK_TRUE, UINT64_MAX);
    device::check(result, "Unable to wait for a vulkan upload");
}

};  // namespace v3d::render::realtime::vulkan::memory

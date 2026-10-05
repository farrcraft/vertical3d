/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Uploader.h"

#include <api/render/realtime/vulkan/device/Result.h>

#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::memory {

/**
 **/
Uploader::Uploader(const boost::shared_ptr<device::Device>& device) :
    device_(device),
    commands_(VK_NULL_HANDLE) {
    pool_ = boost::make_shared<frame::CommandPool>(device_, device_->families().graphics);
    std::vector<VkCommandBuffer> buffers = pool_->allocate(1);
    commands_ = buffers.front();
}

/**
 **/
Uploader::~Uploader() {
    // the buffer is freed with the pool
}

/**
 **/
void Uploader::oneShot(const std::function<void(VkCommandBuffer)>& record) const {
    if (!record) {
        return;
    }

    VkResult result = vkResetCommandBuffer(commands_, 0);
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

    result = vkQueueSubmit2(device_->graphicsQueue(), 1, &submission, VK_NULL_HANDLE);
    device::check(result, "Unable to submit a vulkan upload");

    // the caller's staging allocation goes away when this returns
    vkQueueWaitIdle(device_->graphicsQueue());
}

};  // namespace v3d::render::realtime::vulkan::memory

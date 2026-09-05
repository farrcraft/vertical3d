/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Uploader.h"

#include <sstream>
#include <stdexcept>
#include <vector>

#include "Result.h"

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan {

/**
 **/
Uploader::Uploader(const boost::shared_ptr<Device>& device) :
    device_(device),
    commands_(VK_NULL_HANDLE) {
    pool_ = boost::make_shared<CommandPool>(device_, device_->families().graphics);
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
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to reset the vulkan upload command buffer - " << resultString(result);
        throw std::runtime_error(msg.str());
    }

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(commands_, &begin);
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to begin the vulkan upload command buffer - " << resultString(result);
        throw std::runtime_error(msg.str());
    }

    record(commands_);

    result = vkEndCommandBuffer(commands_);
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to end the vulkan upload command buffer - " << resultString(result);
        throw std::runtime_error(msg.str());
    }

    VkCommandBufferSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    info.commandBuffer = commands_;

    VkSubmitInfo2 submission{};
    submission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submission.commandBufferInfoCount = 1;
    submission.pCommandBufferInfos = &info;

    result = vkQueueSubmit2(device_->graphicsQueue(), 1, &submission, VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to submit a vulkan upload - " << resultString(result);
        throw std::runtime_error(msg.str());
    }

    // the caller's staging allocation goes away when this returns
    vkQueueWaitIdle(device_->graphicsQueue());
}

};  // namespace v3d::render::realtime::vulkan

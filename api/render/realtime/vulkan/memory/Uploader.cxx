/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Uploader.h"

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/device/Result.h>
#include <api/render/realtime/vulkan/frame/CommandPool.h>
#include <api/render/realtime/vulkan/frame/Ring.h>

#include <functional>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::memory {

/**
 **/
Uploader::Uploader(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<frame::Ring>& ring) :
    device_(device),
    ring_(ring),
    spare_(boost::make_shared<Spare>()) {
    spare_->pool = boost::make_shared<frame::CommandPool>(device_, device_->families().graphics);
}

/**
 **/
void Uploader::submit(const std::function<void(VkCommandBuffer)>& record, boost::shared_ptr<void> keep) {
    if (!record) {
        return;
    }

    if (spare_->ready.empty()) {
        spare_->ready = spare_->pool->allocate(1);
    }
    VkCommandBuffer commands = spare_->ready.back();
    spare_->ready.pop_back();

    try {
        VkResult result = vkResetCommandBuffer(commands, 0);
        device::check(result, "Unable to reset a vulkan upload command buffer");

        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        result = vkBeginCommandBuffer(commands, &begin);
        device::check(result, "Unable to begin a vulkan upload command buffer");

        record(commands);

        result = vkEndCommandBuffer(commands);
        device::check(result, "Unable to end a vulkan upload command buffer");

        VkCommandBufferSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        info.commandBuffer = commands;

        VkSubmitInfo2 submission{};
        submission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submission.commandBufferInfoCount = 1;
        submission.pCommandBufferInfos = &info;

        result = vkQueueSubmit2(device_->graphicsQueue(), 1, &submission, VK_NULL_HANDLE);
        device::check(result, "Unable to submit a vulkan upload");
    } catch (...) {
        // nothing was submitted, so the buffer can be used again at once
        spare_->ready.push_back(commands);
        throw;
    }

    // the frame being recorded is submitted after this, and its fence covers this submission,
    // so once it has finished the copy has run
    ring_->retire([spare = spare_, commands, keep = std::move(keep)]() mutable {
        keep.reset();
        spare->ready.push_back(commands);
    });
}

};  // namespace v3d::render::realtime::vulkan::memory

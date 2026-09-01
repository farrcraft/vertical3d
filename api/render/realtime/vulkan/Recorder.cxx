/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Recorder.h"

#include <algorithm>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan {

    /**
     **/
    Recorder::Target::Target() noexcept :
        image(VK_NULL_HANDLE),
        view(VK_NULL_HANDLE),
        extent{0, 0} {
    }

    /**
     **/
    void Recorder::record(VkCommandBuffer commands, const Frame& frame, const Target& target) const {
        // the acquired image comes back in whatever layout it was left in, and nothing in the
        // frame reads it, so undefined is the honest source layout and the cheapest one
        transition(commands, target.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        for (const boost::shared_ptr<Pass>& pass : frame.passes()) {
            record(commands, *pass, target);
        }

        transition(commands, target.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

    /**
     **/
    void Recorder::record(VkCommandBuffer commands, const Pass& pass, const Target& target) {
        VkRenderingAttachmentInfo colour{};
        colour.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colour.imageView = target.view;
        colour.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        // a pass that does not clear draws over what the pass before it left in the image
        colour.loadOp = pass.clears() ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        colour.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colour.clearValue.color.float32[0] = pass.clearColour().r;
        colour.clearValue.color.float32[1] = pass.clearColour().g;
        colour.clearValue.color.float32[2] = pass.clearColour().b;
        colour.clearValue.color.float32[3] = pass.clearColour().a;

        // the region a pass draws into, which is the whole image until something asks for less
        VkRect2D area{};
        area.offset.x = static_cast<int32_t>(pass.viewport().x);
        area.offset.y = static_cast<int32_t>(pass.viewport().y);
        area.extent.width = pass.viewport().z > 0.0f ? static_cast<uint32_t>(pass.viewport().z) : target.extent.width;
        area.extent.height = pass.viewport().w > 0.0f ? static_cast<uint32_t>(pass.viewport().w) : target.extent.height;

        VkRenderingInfo rendering{};
        rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        rendering.renderArea = area;
        rendering.layerCount = 1;
        rendering.colorAttachmentCount = 1;
        rendering.pColorAttachments = &colour;

        vkCmdBeginRendering(commands, &rendering);

        // viewport and scissor are dynamic state everywhere, so a window resize costs no pipelines
        VkViewport viewport{};
        viewport.x = static_cast<float>(area.offset.x);
        viewport.y = static_cast<float>(area.offset.y);
        viewport.width = static_cast<float>(area.extent.width);
        viewport.height = static_cast<float>(area.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commands, 0, 1, &viewport);
        vkCmdSetScissor(commands, 0, 1, &area);

        for (const DrawItem& item : pass.items()) {
            // nothing binds pipelines or descriptor sets yet, so the escape hatch is the
            // only item that draws anything - the rest of the recording lands with the
            // first real pipeline in phase 3
            if (item.record) {
                item.record(commands);
            }
        }

        vkCmdEndRendering(commands);
    }

    /**
     **/
    void Recorder::transition(VkCommandBuffer commands, VkImage image, VkImageLayout from, VkImageLayout to) {
        VkImageMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.oldLayout = from;
        barrier.newLayout = to;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        if (to == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
            // nothing before the attachment writes touches the image, and the submission
            // already waits on the image-available semaphore at that same stage
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_NONE;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        } else {
            // presentation is not a pipeline stage - the semaphore it waits on is what
            // orders it, so the barrier only has to make the writes visible
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_NONE;
        }

        VkDependencyInfo dependency{};
        dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency.imageMemoryBarrierCount = 1;
        dependency.pImageMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(commands, &dependency);
    }

};  // namespace v3d::render::realtime::vulkan

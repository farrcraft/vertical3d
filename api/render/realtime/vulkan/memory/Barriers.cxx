/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Barriers.h"

#include <cstdint>

namespace v3d::render::realtime::vulkan::memory {

namespace {

/**
 * A barrier from one layout to another with no scopes yet, for the caller to fill in.
 **/
VkImageMemoryBarrier2 barrier(VkImage image, VkImageAspectFlags aspect, VkImageLayout from, VkImageLayout to) {
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.oldLayout = from;
    barrier.newLayout = to;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspect;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    return barrier;
}

const VkPipelineStageFlags2 depthTests = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

};  // namespace

/**
 **/
VkImageMemoryBarrier2 colourForDrawing(VkImage image) {
    VkImageMemoryBarrier2 into = barrier(image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    // two things have to have happened before the transition writes the image.
    //
    // COLOR_ATTACHMENT_OUTPUT is the stage the presenter waits the image-available
    // semaphore at, and a transition is a write: without that stage in the first scope
    // the barrier is not ordered after the wait, and the acquire's read of the image
    // races it. Synchronization validation reports that as WRITE_AFTER_READ against
    // vkAcquireNextImageKHR.
    //
    // FRAGMENT_SHADER is for a render target rather than the swapchain: there is one
    // image and two frames in flight, so the previous frame may still be sampling it. A
    // barrier's first scope reaches work already submitted to the queue, so naming the
    // stage that reads is what orders the two.
    into.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    // a write after a read needs the reads to have happened, not to be visible
    into.srcAccessMask = VK_ACCESS_2_NONE;
    into.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    into.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    return into;
}

/**
 **/
VkImageMemoryBarrier2 colourAfterDrawing(VkImage image, VkImageLayout to) {
    VkImageMemoryBarrier2 out = barrier(image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, to);
    out.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    out.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    if (to == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        // a target changing hands: what the pass wrote has to be visible to the fragment
        // shader of whichever later pass samples it
        out.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        out.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    } else {
        // presentation is not a pipeline stage - the semaphore it waits on is what
        // orders it, so the barrier only has to make the writes visible
        out.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        out.dstAccessMask = VK_ACCESS_2_NONE;
    }
    return out;
}

/**
 **/
VkImageMemoryBarrier2 depthForDrawing(VkImage image) {
    VkImageMemoryBarrier2 into = barrier(image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    // one depth image serves every frame in flight, so this transition lands on top of the
    // last frame's storeOp. A layout transition is a write of its own, and the access bit is
    // what makes the earlier write available to it - a stage on its own orders nothing.
    // Depth is written at both fragment test stages
    into.srcStageMask = depthTests;
    into.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    into.dstStageMask = depthTests;
    into.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    return into;
}

/**
 **/
VkImageMemoryBarrier2 depthForSampling(VkImage image) {
    VkImageMemoryBarrier2 out = barrier(image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);
    // the writes being waited on are the depth tests of the pass that just ran, and what
    // waits on them is a fragment shader sampling the result
    out.srcStageMask = depthTests;
    out.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    out.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    out.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    return out;
}

/**
 **/
VkImageMemoryBarrier2 forUpload(VkImage image) {
    VkImageMemoryBarrier2 into = barrier(image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    into.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    into.srcAccessMask = VK_ACCESS_2_NONE;
    into.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    into.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    return into;
}

/**
 **/
VkImageMemoryBarrier2 uploadedForSampling(VkImage image) {
    VkImageMemoryBarrier2 out = barrier(image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    out.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    out.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    out.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    out.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    return out;
}

/**
 **/
VkImageMemoryBarrier2 forReadback(VkImage image, VkImageAspectFlags aspect, VkImageLayout from) {
    VkImageMemoryBarrier2 into = barrier(image, aspect, from, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    // ALL_COMMANDS rather than the stage that drew: what this has to be ordered after is
    // whatever transitioned the image into PRESENT_SRC, and a capture cannot tell which
    // barrier that was or which stage it named as its second scope. What has to be made
    // visible is still only the frame's own writes.
    into.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    into.srcAccessMask = (aspect & VK_IMAGE_ASPECT_DEPTH_BIT) != 0
        ? VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
        : VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    into.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    into.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
    return into;
}

/**
 **/
VkImageMemoryBarrier2 afterReadback(VkImage image, VkImageAspectFlags aspect, VkImageLayout to) {
    VkImageMemoryBarrier2 out = barrier(image, aspect, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, to);
    // a transition is a write and the copy was a read, so what this needs is for the read
    // to have happened rather than to be visible
    out.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    out.srcAccessMask = VK_ACCESS_2_NONE;
    if (to == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        // presentation is not a pipeline stage - the semaphore it waits on is what orders
        // it, so the barrier only has to put the image back in the layout it expects
        out.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        out.dstAccessMask = VK_ACCESS_2_NONE;
    } else {
        // anything else is going back to a layout something in this same submit may sample
        // or draw into, and unlike presentation that use has no semaphore of its own to
        // order it. The transition is a write, so it has to be complete and visible before
        // any of them rather than merely before the end of the buffer.
        out.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        out.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    }
    return out;
}

/**
 **/
void record(VkCommandBuffer commands, std::initializer_list<VkImageMemoryBarrier2> barriers) {
    VkDependencyInfo dependency{};
    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size());
    dependency.pImageMemoryBarriers = barriers.begin();
    vkCmdPipelineBarrier2(commands, &dependency);
}

};  // namespace v3d::render::realtime::vulkan::memory

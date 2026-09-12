/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Capture.h"

#include <api/image/Image.h>
#include <api/image/writer/Png.h>

#include "Swapchain.h"

#include <string>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::frame {

namespace {

/**
 * The two scopes a readback needs, either side of the copy.
 **/
void transition(VkCommandBuffer commands, VkImage image, VkImageLayout from, VkImageLayout to) {
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

    if (to == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        // ALL_COMMANDS rather than the stage that drew: what this has to be ordered after is
        // whatever transitioned the image into PRESENT_SRC, and a capture cannot know which
        // barrier that was or which stage it named as its second scope. What has to be made
        // visible is still only the frame's own writes.
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
    } else if (to == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        // presentation is not a pipeline stage - the semaphore it waits on is what orders
        // it, so the barrier only has to put the image back in the layout it expects. A
        // transition is a write and the copy was a read, so what this needs is for the read
        // to have happened rather than to be visible.
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_NONE;
    } else {
        // anything else is going back to a layout something in this same submit may sample
        // or draw into, and unlike presentation that use has no semaphore of its own to
        // order it. The transition is a write, so it has to be complete and visible before
        // any of them rather than merely before the end of the buffer.
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    }

    VkDependencyInfo dependency{};
    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency.imageMemoryBarrierCount = 1;
    dependency.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(commands, &dependency);
}

};  // namespace

/**
 **/
Capture::Capture(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<log::Logger>& logger) :
    device_(device),
    logger_(logger),
    width_(0),
    height_(0),
    format_(VK_FORMAT_UNDEFINED) {
}

/**
 **/
Capture::Source::Source() noexcept :
image(VK_NULL_HANDLE),
extent{0, 0},
format(VK_FORMAT_UNDEFINED),
layout(VK_IMAGE_LAYOUT_UNDEFINED) {
}

/**
 **/
void Capture::record(VkCommandBuffer commands, const Source& source) {
    width_ = source.extent.width;
    height_ = source.extent.height;
    format_ = source.format;

    const VkDeviceSize bytes = static_cast<VkDeviceSize>(width_) * height_ * 4;
    if (!readback_) {
        readback_ = boost::make_shared<memory::Buffer>(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT, bytes);
    } else {
        readback_->grow(bytes);
    }

    transition(commands, source.image, source.layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width_, height_, 1};
    vkCmdCopyImageToBuffer(commands, source.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback_->handle(), 1, &region);

    transition(commands, source.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, source.layout);
}

/**
 **/
void Capture::record(VkCommandBuffer commands, const Swapchain& swapchain, uint32_t image) {
    Source source;
    source.image = swapchain.images()[image];
    source.extent = swapchain.extent();
    source.format = swapchain.format();
    source.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    record(commands, source);
}

/**
 **/
bool Capture::write(std::string_view filename) {
    if (!readback_ || width_ == 0 || height_ == 0) {
        return false;
    }

    std::vector<unsigned char> pixels(static_cast<std::size_t>(width_) * height_ * 4);
    readback_->read(pixels.data(), static_cast<VkDeviceSize>(pixels.size()));

    image::writer::Png png(logger_);
    if (!png.write(filename, convert(pixels.data(), width_, height_, format_))) {
        return false;
    }

    logger_->get()->info("Wrote a {} x {} capture to {}", width_, height_, std::string(filename));
    return true;
}

/**
 **/
boost::shared_ptr<image::Image> Capture::convert(const unsigned char* pixels, uint32_t width, uint32_t height,
    VkFormat format) {
    boost::shared_ptr<image::Image> img = boost::make_shared<image::Image>(width, height, 32);

    const bool swizzle = format == VK_FORMAT_B8G8R8A8_UNORM || format == VK_FORMAT_B8G8R8A8_SRGB;

    unsigned char* out = img->data();
    const std::size_t texels = static_cast<std::size_t>(width) * height;
    for (std::size_t i = 0; i < texels; i++) {
        const std::size_t at = i * 4;
        out[at + 0] = swizzle ? pixels[at + 2] : pixels[at + 0];
        out[at + 1] = pixels[at + 1];
        out[at + 2] = swizzle ? pixels[at + 0] : pixels[at + 2];
        out[at + 3] = 255;
    }

    return img;
}

};  // namespace v3d::render::realtime::vulkan::frame

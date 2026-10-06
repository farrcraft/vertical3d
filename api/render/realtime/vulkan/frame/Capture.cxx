/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Capture.h"

#include <api/image/Image.h>
#include <api/image/writer/Png.h>
#include <api/render/realtime/vulkan/memory/Barriers.h>

#include "Swapchain.h"

#include <stdexcept>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 **/
Capture::Capture(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<log::Logger>& logger) :
    device_(device),
    logger_(logger),
    width_(0),
    height_(0),
    format_(VK_FORMAT_UNDEFINED),
    depth_(false) {
}

/**
 **/
Capture::Source::Source() noexcept :
image(VK_NULL_HANDLE),
extent{0, 0},
format(VK_FORMAT_UNDEFINED),
layout(VK_IMAGE_LAYOUT_UNDEFINED),
depth(false) {
}

/**
 **/
void Capture::record(VkCommandBuffer commands, const Source& source) {
    if (source.depth && source.format != VK_FORMAT_D32_SFLOAT) {
        throw std::runtime_error("A depth capture can only read a D32_SFLOAT image");
    }
    depth_ = source.depth;
    width_ = source.extent.width;
    height_ = source.extent.height;
    format_ = source.format;

    const VkDeviceSize bytes = static_cast<VkDeviceSize>(width_) * height_ * 4;
    if (!readback_) {
        readback_ = boost::make_shared<memory::Buffer>(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT, bytes);
    } else {
        readback_->grow(bytes);
    }

    const VkImageAspectFlags aspect = depth_ ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    memory::record(commands, {memory::forReadback(source.image, aspect, source.layout)});

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = aspect;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width_, height_, 1};
    vkCmdCopyImageToBuffer(commands, source.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback_->handle(), 1, &region);

    memory::record(commands, {memory::afterReadback(source.image, aspect, source.layout)});
}

/**
 **/
void Capture::record(VkCommandBuffer commands, const Swapchain& swapchain, uint32_t image) {
    if (!swapchain.copyable()) {
        throw std::runtime_error("The swapchain images were created without TRANSFER_SRC usage and cannot be captured");
    }
    if (image >= swapchain.images().size()) {
        throw std::runtime_error("A swapchain capture names image " + std::to_string(image) + " of a chain of " +
            std::to_string(swapchain.images().size()));
    }
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
    if (!readback_ || width_ == 0 || height_ == 0 || depth_) {
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
std::vector<float> Capture::depth() const {
    if (!readback_ || !depth_) {
        return std::vector<float>();
    }
    std::vector<float> depths(static_cast<std::size_t>(width_) * height_);
    readback_->read(depths.data(), static_cast<VkDeviceSize>(depths.size() * sizeof(float)));
    return depths;
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

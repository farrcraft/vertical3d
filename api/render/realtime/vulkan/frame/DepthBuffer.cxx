/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "DepthBuffer.h"

#include <stdexcept>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::frame {

namespace {

/**
 * In preference order. A depth only format is what a scene wants - nothing in the
 * renderer stencils - so the two combined formats are here only for a device that
 * cannot use D32 as an attachment.
 **/
const VkFormat candidates[3] = {
    VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_D24_UNORM_S8_UINT
};

};  // namespace

/**
 **/
DepthBuffer::DepthBuffer(const boost::shared_ptr<device::Device>& device, uint32_t width, uint32_t height,
    bool sampled) :
    device_(device),
    format_(VK_FORMAT_UNDEFINED),
    extent_(),
    sampled_(sampled) {
    format_ = chooseFormat(device_->physical(), sampled_);
    create(width, height);
}

/**
 **/
DepthBuffer::~DepthBuffer() {
    destroy();
}

/**
 **/
VkFormat DepthBuffer::chooseFormat(VkPhysicalDevice device, bool sampled) {
    VkFormatFeatureFlags wanted = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (sampled) {
        // a format a device will draw depth into is not necessarily one it will let a
        // shader read, so asking for both narrows the list rather than only the usage
        wanted |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    }
    for (VkFormat format : candidates) {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(device, format, &properties);
        if ((properties.optimalTilingFeatures & wanted) == wanted) {
            return format;
        }
    }
    if (sampled) {
        throw std::runtime_error("No vulkan depth format the device offers can be both drawn into and sampled");
    }
    throw std::runtime_error("No vulkan depth format the device offers can be used as an attachment");
}

/**
 **/
void DepthBuffer::create(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
        // a minimized window, which the swapchain handles the same way
        return;
    }

    memory::Image::Spec spec;
    spec.width = width;
    spec.height = height;
    spec.format = format_;
    spec.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (sampled_) {
        // TRANSFER_SRC on the same terms a render target's colour has it: what lets
        // frame::Capture read a shadow map back, for the price of whatever compression a
        // driver declines on an image that can be copied out
        spec.usage |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    // the stencil aspect is left out even where the format carries one - nothing
    // stencils, and an attachment view may name only the aspects it is used through
    spec.aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    image_ = boost::make_shared<memory::Image>(device_, spec);

    if (sampled_) {
        pipeline::Sampler::Spec sampler;
        // linear, so that a shadow comparison across a texel boundary softens rather than
        // stepping. Nothing here enables the compare mode: a caller that wants a hardware
        // pcf sampler wants its own, and this is the one a plain read uses
        sampler.mipmap = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        // outside what was rendered is lit, not shadowed, so the border is the far plane
        sampler.address = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        sampler.border = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sampler.maxLod = 1.0f;
        sampler_ = boost::make_shared<pipeline::Sampler>(device_, sampler);
    }

    extent_.width = width;
    extent_.height = height;
}

/**
 **/
void DepthBuffer::destroy() {
    sampler_.reset();
    image_.reset();
    extent_.width = 0;
    extent_.height = 0;
}

/**
 **/
void DepthBuffer::recreate(uint32_t width, uint32_t height) {
    // a frame in flight may still be testing against the image about to go away
    vkDeviceWaitIdle(device_->handle());
    destroy();
    create(width, height);
}

/**
 **/
bool DepthBuffer::valid() const noexcept {
    return static_cast<bool>(image_);
}

/**
 **/
VkImage DepthBuffer::image() const noexcept {
    return image_ ? image_->handle() : VK_NULL_HANDLE;
}

/**
 **/
VkImageView DepthBuffer::view() const noexcept {
    return image_ ? image_->view() : VK_NULL_HANDLE;
}

/**
 **/
VkFormat DepthBuffer::format() const noexcept {
    return format_;
}

/**
 **/
const VkExtent2D& DepthBuffer::extent() const noexcept {
    return extent_;
}

/**
 **/
bool DepthBuffer::stencil() const noexcept {
    return format_ == VK_FORMAT_D32_SFLOAT_S8_UINT || format_ == VK_FORMAT_D24_UNORM_S8_UINT;
}

/**
 **/
bool DepthBuffer::sampled() const noexcept {
    return sampled_;
}

/**
 **/
VkSampler DepthBuffer::sampler() const noexcept {
    return sampler_ ? sampler_->handle() : VK_NULL_HANDLE;
}

/**
 **/
pipeline::Texture DepthBuffer::texture() const {
    pipeline::Texture texture;
    if (!sampled_ || !image_) {
        return texture;
    }
    texture.image = image_;
    texture.sampler = sampler_;
    return texture;
}

};  // namespace v3d::render::realtime::vulkan::frame

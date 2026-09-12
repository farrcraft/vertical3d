/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RenderTarget.h"

#include <api/render/realtime/vulkan/device/Result.h>
#include <api/render/realtime/vulkan/memory/Memory.h>

#include <sstream>
#include <stdexcept>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 **/
RenderTarget::RenderTarget(const boost::shared_ptr<device::Device>& device, uint32_t width, uint32_t height,
    VkFormat colour, bool depth, bool sampledDepth) :
    device_(device),
    format_(colour),
    image_(VK_NULL_HANDLE),
    view_(VK_NULL_HANDLE),
    sampler_(VK_NULL_HANDLE),
    extent_(),
    wantsDepth_(depth),
    sampledDepth_(sampledDepth) {
    create(width, height);
}

/**
 **/
RenderTarget::~RenderTarget() {
    destroy();
}

/**
 **/
void RenderTarget::recreate(uint32_t width, uint32_t height) {
    destroy();
    create(width, height);
}

/**
 **/
void RenderTarget::create(uint32_t width, uint32_t height) {
    // unlike a swapchain image, a target is asked for at a size the caller chose, so a
    // dimension of zero is a mistake rather than a minimized window
    if (width == 0 || height == 0) {
        throw std::runtime_error("A vulkan render target cannot have a zero dimension");
    }

    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format_;
    info.extent.width = width;
    info.extent.height = height;
    info.extent.depth = 1;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    // both halves of what a target is for: a pass draws into it and a later pass reads it.
    // TRANSFER_SRC is what lets frame::Capture copy one out, and is granted rather than asked
    // for on the same terms SAMPLED is - a colour image that cannot be read is the narrower
    // thing to be, and the only cost here is whichever compression a desktop driver declines
    info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult result = vkCreateImage(device_->handle(), &info, nullptr, &image_);
    if (result != VK_SUCCESS) {
        image_ = VK_NULL_HANDLE;
        std::stringstream msg;
        msg << "Unable to create a vulkan render target image - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    result = device_->allocator().bind(image_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memory_);
    if (result != VK_SUCCESS) {
        destroy();
        std::stringstream msg;
        msg << "Unable to allocate memory for a vulkan render target - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    VkImageViewCreateInfo view{};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.image = image_;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = format_;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.layerCount = 1;

    result = vkCreateImageView(device_->handle(), &view, nullptr, &view_);
    if (result != VK_SUCCESS) {
        view_ = VK_NULL_HANDLE;
        destroy();
        std::stringstream msg;
        msg << "Unable to create a vulkan render target view - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    VkSamplerCreateInfo sampler{};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // linear, like every other sampler in the renderer: a target is read at whatever size
    // the pass reading it draws, which is rarely the size it was rendered at
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    // clamping rather than repeating: sampling past the edge of a target is a shader
    // reaching outside what was rendered, and wrapping would answer with the far side
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    sampler.maxLod = VK_LOD_CLAMP_NONE;

    result = vkCreateSampler(device_->handle(), &sampler, nullptr, &sampler_);
    if (result != VK_SUCCESS) {
        sampler_ = VK_NULL_HANDLE;
        destroy();
        std::stringstream msg;
        msg << "Unable to create a vulkan render target sampler - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    if (wantsDepth_) {
        // a depth buffer is the same image at the same size whoever is drawing into it, so
        // a target's is one of those rather than a second implementation of the same thing
        depth_ = boost::make_shared<DepthBuffer>(device_, width, height, sampledDepth_);
    }

    extent_.width = width;
    extent_.height = height;
}

/**
 **/
void RenderTarget::destroy() {
    depth_.reset();
    if (sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device_->handle(), sampler_, nullptr);
        sampler_ = VK_NULL_HANDLE;
    }
    if (view_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device_->handle(), view_, nullptr);
        view_ = VK_NULL_HANDLE;
    }
    device_->allocator().free(&memory_);
    if (image_ != VK_NULL_HANDLE) {
        vkDestroyImage(device_->handle(), image_, nullptr);
        image_ = VK_NULL_HANDLE;
    }
    extent_.width = 0;
    extent_.height = 0;
}

/**
 **/
VkImage RenderTarget::image() const noexcept {
    return image_;
}

/**
 **/
VkImageView RenderTarget::view() const noexcept {
    return view_;
}

/**
 **/
VkSampler RenderTarget::sampler() const noexcept {
    return sampler_;
}

/**
 **/
VkFormat RenderTarget::format() const noexcept {
    return format_;
}

/**
 **/
const VkExtent2D& RenderTarget::extent() const noexcept {
    return extent_;
}

/**
 **/
VkImage RenderTarget::depthImage() const noexcept {
    return depth_ ? depth_->image() : VK_NULL_HANDLE;
}

/**
 **/
VkImageView RenderTarget::depthView() const noexcept {
    return depth_ ? depth_->view() : VK_NULL_HANDLE;
}

/**
 **/
VkFormat RenderTarget::depthFormat() const noexcept {
    return depth_ ? depth_->format() : VK_FORMAT_UNDEFINED;
}

/**
 **/
bool RenderTarget::sampledDepth() const noexcept {
    return sampledDepth_ && depth_ && depth_->sampled();
}

/**
 **/
pipeline::Texture RenderTarget::depthTexture() const {
    pipeline::Texture texture;
    if (!sampledDepth()) {
        return texture;
    }
    texture.image = depth_->image();
    texture.view = depth_->view();
    texture.sampler = depth_->sampler();
    texture.extent = depth_->extent();
    // borrowed the same way the colour image is - the buffer owns them and rebuilds them
    // whenever the target is resized, so the allocation stays the empty one
    texture.owned = false;
    return texture;
}

/**
 **/
pipeline::Texture RenderTarget::texture() const {
    pipeline::Texture texture;
    texture.image = image_;
    texture.view = view_;
    texture.sampler = sampler_;
    texture.extent = extent_;
    // the allocation, the view and the sampler are the target's and are thrown away every
    // time it is resized, so what is registered names them rather than taking them over
    texture.owned = false;
    return texture;
}

};  // namespace v3d::render::realtime::vulkan::frame

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RenderTarget.h"

#include <sstream>
#include <stdexcept>

#include "Memory.h"
#include "Result.h"

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan {

/**
 **/
RenderTarget::RenderTarget(const boost::shared_ptr<Device>& device, uint32_t width, uint32_t height,
    VkFormat colour, bool depth) :
    device_(device),
    format_(colour),
    image_(VK_NULL_HANDLE),
    memory_(VK_NULL_HANDLE),
    view_(VK_NULL_HANDLE),
    sampler_(VK_NULL_HANDLE),
    extent_(),
    wantsDepth_(depth) {
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
    // both halves of what a target is for: a pass draws into it and a later pass reads it
    info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult result = vkCreateImage(device_->handle(), &info, nullptr, &image_);
    if (result != VK_SUCCESS) {
        image_ = VK_NULL_HANDLE;
        std::stringstream msg;
        msg << "Unable to create a vulkan render target image - " << resultString(result);
        throw std::runtime_error(msg.str());
    }

    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device_->handle(), image_, &requirements);

    VkMemoryAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex =
        memoryType(device_->physical(), requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    result = vkAllocateMemory(device_->handle(), &allocation, nullptr, &memory_);
    if (result != VK_SUCCESS) {
        memory_ = VK_NULL_HANDLE;
        destroy();
        std::stringstream msg;
        msg << "Unable to allocate memory for a vulkan render target - " << resultString(result);
        throw std::runtime_error(msg.str());
    }

    result = vkBindImageMemory(device_->handle(), image_, memory_, 0);
    if (result != VK_SUCCESS) {
        destroy();
        std::stringstream msg;
        msg << "Unable to bind memory to a vulkan render target - " << resultString(result);
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
        msg << "Unable to create a vulkan render target view - " << resultString(result);
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
        msg << "Unable to create a vulkan render target sampler - " << resultString(result);
        throw std::runtime_error(msg.str());
    }

    if (wantsDepth_) {
        // a depth buffer is the same image at the same size whoever is drawing into it, so
        // a target's is one of those rather than a second implementation of the same thing
        depth_ = boost::make_shared<DepthBuffer>(device_, width, height);
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
    if (memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_->handle(), memory_, nullptr);
        memory_ = VK_NULL_HANDLE;
    }
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
Texture RenderTarget::texture() const {
    Texture texture;
    texture.image = image_;
    texture.view = view_;
    texture.sampler = sampler_;
    texture.extent = extent_;
    // the allocation, the view and the sampler are the target's and are thrown away every
    // time it is resized, so what is registered names them rather than taking them over
    texture.memory = VK_NULL_HANDLE;
    texture.owned = false;
    return texture;
}

};  // namespace v3d::render::realtime::vulkan

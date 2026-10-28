/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Image.h"

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/device/Result.h>

#include <cstdint>
#include <stdexcept>

#include "Allocator.h"

namespace v3d::render::realtime::vulkan::memory {

/**
 **/
Image::Spec::Spec() noexcept :
width(0),
height(0),
depth(1),
format(VK_FORMAT_UNDEFINED),
usage(0),
aspect(VK_IMAGE_ASPECT_COLOR_BIT),
mipLevels(1),
components{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
layers(1),
arrayed(false) {
}

/**
 **/
Image::Image(const boost::shared_ptr<device::Device>& device, const Spec& spec) :
    device_(device),
    spec_(spec),
    image_(VK_NULL_HANDLE),
    view_(VK_NULL_HANDLE) {
    if (spec_.width == 0 || spec_.height == 0 || spec_.depth == 0 || spec_.layers == 0) {
        throw std::runtime_error("A vulkan image cannot have a zero dimension");
    }
    const bool volume = spec_.depth > 1;
    if (volume) {
        spec_.layers = 1;
        spec_.arrayed = false;
    }
    if (spec_.layers > 1) {
        spec_.arrayed = true;
    }

    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = volume ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    info.format = spec_.format;
    info.extent.width = spec_.width;
    info.extent.height = spec_.height;
    info.extent.depth = spec_.depth;
    info.mipLevels = spec_.mipLevels;
    info.arrayLayers = spec_.layers;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = spec_.usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult result = vkCreateImage(device_->handle(), &info, nullptr, &image_);
    if (result != VK_SUCCESS) {
        image_ = VK_NULL_HANDLE;
        throw device::failure(result, "Unable to create a vulkan image");
    }

    result = device_->allocator().bind(image_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memory_);
    if (result != VK_SUCCESS) {
        destroy();
        throw device::failure(result, "Unable to allocate memory for a vulkan image");
    }

    VkImageViewCreateInfo view{};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.image = image_;
    if (volume) {
        view.viewType = VK_IMAGE_VIEW_TYPE_3D;
    } else {
        view.viewType = spec_.arrayed ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    }
    view.format = spec_.format;
    view.components = spec_.components;
    view.subresourceRange.aspectMask = spec_.aspect;
    view.subresourceRange.levelCount = spec_.mipLevels;
    view.subresourceRange.layerCount = spec_.layers;

    result = vkCreateImageView(device_->handle(), &view, nullptr, &view_);
    if (result != VK_SUCCESS) {
        view_ = VK_NULL_HANDLE;
        destroy();
        throw device::failure(result, "Unable to create a vulkan image view");
    }

    if (!spec_.arrayed) {
        return;
    }
    // an attachment draws into one layer through a 2D view of it
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.subresourceRange.layerCount = 1;
    for (uint32_t layer = 0; layer < spec_.layers; layer++) {
        view.subresourceRange.baseArrayLayer = layer;
        VkImageView single = VK_NULL_HANDLE;
        result = vkCreateImageView(device_->handle(), &view, nullptr, &single);
        if (result != VK_SUCCESS) {
            destroy();
            throw device::failure(result, "Unable to create a vulkan image view of one layer");
        }
        layerViews_.push_back(single);
    }
}

/**
 **/
Image::~Image() {
    destroy();
}

/**
 **/
void Image::destroy() noexcept {
    for (const auto& single : layerViews_) {
        vkDestroyImageView(device_->handle(), single, nullptr);
    }
    layerViews_.clear();
    if (view_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device_->handle(), view_, nullptr);
        view_ = VK_NULL_HANDLE;
    }
    if (image_ != VK_NULL_HANDLE) {
        vkDestroyImage(device_->handle(), image_, nullptr);
        image_ = VK_NULL_HANDLE;
    }
    // free the memory last, after the image it backs
    device_->allocator().free(&memory_);
}

/**
 **/
VkImage Image::handle() const noexcept {
    return image_;
}

/**
 **/
VkImageView Image::view() const noexcept {
    return view_;
}

/**
 **/
VkImageView Image::layerView(uint32_t layer) const noexcept {
    if (!spec_.arrayed) {
        return layer == 0 ? view_ : VK_NULL_HANDLE;
    }
    return layer < layerViews_.size() ? layerViews_[layer] : VK_NULL_HANDLE;
}

/**
 **/
uint32_t Image::layers() const noexcept {
    return spec_.layers;
}

/**
 **/
const Image::Spec& Image::spec() const noexcept {
    return spec_;
}

/**
 **/
VkExtent2D Image::extent() const noexcept {
    return VkExtent2D{spec_.width, spec_.height};
}

};  // namespace v3d::render::realtime::vulkan::memory

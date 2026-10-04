/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Image.h"

#include <api/render/realtime/vulkan/device/Result.h>

#include <sstream>
#include <stdexcept>

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
components{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
    VK_COMPONENT_SWIZZLE_IDENTITY} {
}

/**
 **/
Image::Image(const boost::shared_ptr<device::Device>& device, const Spec& spec) :
    device_(device),
    spec_(spec),
    image_(VK_NULL_HANDLE),
    view_(VK_NULL_HANDLE) {
    if (spec_.width == 0 || spec_.height == 0 || spec_.depth == 0) {
        throw std::runtime_error("A vulkan image cannot have a zero dimension");
    }
    const bool volume = spec_.depth > 1;

    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = volume ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    info.format = spec_.format;
    info.extent.width = spec_.width;
    info.extent.height = spec_.height;
    info.extent.depth = spec_.depth;
    info.mipLevels = spec_.mipLevels;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = spec_.usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult result = vkCreateImage(device_->handle(), &info, nullptr, &image_);
    if (result != VK_SUCCESS) {
        image_ = VK_NULL_HANDLE;
        std::stringstream msg;
        msg << "Unable to create a vulkan image - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    result = device_->allocator().bind(image_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memory_);
    if (result != VK_SUCCESS) {
        destroy();
        std::stringstream msg;
        msg << "Unable to allocate memory for a vulkan image - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    VkImageViewCreateInfo view{};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.image = image_;
    view.viewType = volume ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D;
    view.format = spec_.format;
    view.components = spec_.components;
    view.subresourceRange.aspectMask = spec_.aspect;
    view.subresourceRange.levelCount = spec_.mipLevels;
    view.subresourceRange.layerCount = 1;

    result = vkCreateImageView(device_->handle(), &view, nullptr, &view_);
    if (result != VK_SUCCESS) {
        view_ = VK_NULL_HANDLE;
        destroy();
        std::stringstream msg;
        msg << "Unable to create a vulkan image view - " << device::resultString(result);
        throw std::runtime_error(msg.str());
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
    if (view_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device_->handle(), view_, nullptr);
        view_ = VK_NULL_HANDLE;
    }
    if (image_ != VK_NULL_HANDLE) {
        vkDestroyImage(device_->handle(), image_, nullptr);
        image_ = VK_NULL_HANDLE;
    }
    // the memory outlives the image it backs, so it goes last
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
const Image::Spec& Image::spec() const noexcept {
    return spec_;
}

/**
 **/
VkExtent2D Image::extent() const noexcept {
    return VkExtent2D{spec_.width, spec_.height};
}

};  // namespace v3d::render::realtime::vulkan::memory

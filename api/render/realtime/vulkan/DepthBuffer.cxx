/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "DepthBuffer.h"

#include <sstream>
#include <stdexcept>

#include "Memory.h"
#include "Result.h"

namespace v3d::render::realtime::vulkan {

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
    DepthBuffer::DepthBuffer(const boost::shared_ptr<Device>& device, uint32_t width, uint32_t height) :
        device_(device),
        format_(VK_FORMAT_UNDEFINED),
        image_(VK_NULL_HANDLE),
        memory_(VK_NULL_HANDLE),
        view_(VK_NULL_HANDLE),
        extent_{0, 0} {
        format_ = chooseFormat(device_->physical());
        create(width, height);
    }

    /**
     **/
    DepthBuffer::~DepthBuffer() {
        destroy();
    }

    /**
     **/
    VkFormat DepthBuffer::chooseFormat(VkPhysicalDevice device) {
        for (VkFormat format : candidates) {
            VkFormatProperties properties{};
            vkGetPhysicalDeviceFormatProperties(device, format, &properties);
            if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0) {
                return format;
            }
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
        info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkResult result = vkCreateImage(device_->handle(), &info, nullptr, &image_);
        if (result != VK_SUCCESS) {
            image_ = VK_NULL_HANDLE;
            std::stringstream msg;
            msg << "Unable to create the vulkan depth image - " << resultString(result);
            throw std::runtime_error(msg.str());
        }

        VkMemoryRequirements requirements{};
        vkGetImageMemoryRequirements(device_->handle(), image_, &requirements);

        VkMemoryAllocateInfo allocation{};
        allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = memoryType(device_->physical(), requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        result = vkAllocateMemory(device_->handle(), &allocation, nullptr, &memory_);
        if (result != VK_SUCCESS) {
            memory_ = VK_NULL_HANDLE;
            destroy();
            std::stringstream msg;
            msg << "Unable to allocate memory for the vulkan depth image - " << resultString(result);
            throw std::runtime_error(msg.str());
        }

        result = vkBindImageMemory(device_->handle(), image_, memory_, 0);
        if (result != VK_SUCCESS) {
            destroy();
            std::stringstream msg;
            msg << "Unable to bind memory to the vulkan depth image - " << resultString(result);
            throw std::runtime_error(msg.str());
        }

        VkImageViewCreateInfo view{};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.image = image_;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = format_;
        // the stencil aspect is left out even where the format carries one - nothing
        // stencils, and an attachment view may name only the aspects it is used through
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        view.subresourceRange.levelCount = 1;
        view.subresourceRange.layerCount = 1;

        result = vkCreateImageView(device_->handle(), &view, nullptr, &view_);
        if (result != VK_SUCCESS) {
            view_ = VK_NULL_HANDLE;
            destroy();
            std::stringstream msg;
            msg << "Unable to create the vulkan depth image view - " << resultString(result);
            throw std::runtime_error(msg.str());
        }

        extent_.width = width;
        extent_.height = height;
    }

    /**
     **/
    void DepthBuffer::destroy() {
        if (view_ != VK_NULL_HANDLE) {
            vkDestroyImageView(device_->handle(), view_, nullptr);
            view_ = VK_NULL_HANDLE;
        }
        if (image_ != VK_NULL_HANDLE) {
            vkDestroyImage(device_->handle(), image_, nullptr);
            image_ = VK_NULL_HANDLE;
        }
        if (memory_ != VK_NULL_HANDLE) {
            vkFreeMemory(device_->handle(), memory_, nullptr);
            memory_ = VK_NULL_HANDLE;
        }
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
        return view_ != VK_NULL_HANDLE;
    }

    /**
     **/
    VkImage DepthBuffer::image() const noexcept {
        return image_;
    }

    /**
     **/
    VkImageView DepthBuffer::view() const noexcept {
        return view_;
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

};  // namespace v3d::render::realtime::vulkan

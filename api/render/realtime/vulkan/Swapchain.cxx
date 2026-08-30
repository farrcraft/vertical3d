/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Swapchain.h"

#include "Result.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace v3d::render::realtime::vulkan {

    /**
     **/
    Swapchain::Swapchain(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Device>& device, uint32_t width, uint32_t height) :
        device_(device),
        logger_(logger),
        swapchain_(VK_NULL_HANDLE),
        format_(VK_FORMAT_UNDEFINED),
        extent_{0, 0} {
        try {
            create(width, height);
            createViews();
        } catch (...) {
            // nothing runs the destructor of an object whose constructor threw
            destroy();
            throw;
        }
    }

    /**
     **/
    Swapchain::~Swapchain() {
        destroy();
    }

    /**
     **/
    void Swapchain::recreate(uint32_t width, uint32_t height) {
        // the images cannot go away while the queues are still reading them
        vkDeviceWaitIdle(device_->handle());
        destroy();
        create(width, height);
        createViews();
    }

    /**
     **/
    bool Swapchain::valid() const noexcept {
        return swapchain_ != VK_NULL_HANDLE;
    }

    /**
     **/
    VkSwapchainKHR Swapchain::handle() const noexcept {
        return swapchain_;
    }

    /**
     **/
    VkFormat Swapchain::format() const noexcept {
        return format_;
    }

    /**
     **/
    const VkExtent2D& Swapchain::extent() const noexcept {
        return extent_;
    }

    /**
     **/
    const std::vector<VkImageView>& Swapchain::views() const noexcept {
        return views_;
    }

    /**
     **/
    std::size_t Swapchain::length() const noexcept {
        return images_.size();
    }

    /**
     **/
    Swapchain::Support Swapchain::querySupport() const {
        Support support{};
        VkPhysicalDevice physical = device_->physical();
        VkSurfaceKHR surface = device_->surface()->handle();

        VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &support.capabilities);
        if (result != VK_SUCCESS) {
            std::stringstream msg;
            msg << "Unable to read the vulkan surface capabilities - " << resultString(result);
            throw std::runtime_error(msg.str());
        }

        uint32_t formatCount = 0;
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &formatCount, nullptr);
        if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
            std::stringstream msg;
            msg << "Unable to count the vulkan surface formats - " << resultString(result);
            throw std::runtime_error(msg.str());
        }
        support.formats.resize(formatCount);
        if (formatCount > 0) {
            vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &formatCount, support.formats.data());
        }

        uint32_t modeCount = 0;
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &modeCount, nullptr);
        if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
            std::stringstream msg;
            msg << "Unable to count the vulkan present modes - " << resultString(result);
            throw std::runtime_error(msg.str());
        }
        support.presentModes.resize(modeCount);
        if (modeCount > 0) {
            vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &modeCount, support.presentModes.data());
        }

        if (support.formats.empty() || support.presentModes.empty()) {
            throw std::runtime_error("The vulkan surface offers no format or no present mode to draw with");
        }

        return support;
    }

    /**
     **/
    VkSurfaceFormatKHR Swapchain::chooseFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
        for (const VkSurfaceFormatKHR& format : formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
        return formats.front();
    }

    /**
     **/
    VkPresentModeKHR Swapchain::choosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
        for (VkPresentModeKHR mode : modes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return mode;
            }
        }
        // fifo is the only mode every implementation has to offer
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    /**
     **/
    VkExtent2D Swapchain::chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) {
        // a surface that has already settled on a size does not let us pick one
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        }

        VkExtent2D extent{};
        extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return extent;
    }

    /**
     **/
    void Swapchain::create(uint32_t width, uint32_t height) {
        const Support support = querySupport();

        const VkSurfaceFormatKHR surfaceFormat = chooseFormat(support.formats);
        const VkPresentModeKHR presentMode = choosePresentMode(support.presentModes);
        const VkExtent2D extent = chooseExtent(support.capabilities, width, height);

        // a window with no area has no chain - leave it empty and let the caller come back
        if (extent.width == 0 || extent.height == 0) {
            format_ = surfaceFormat.format;
            extent_ = extent;
            logger_->get()->info("Window has no area, leaving the vulkan swapchain empty");
            return;
        }

        // one more than the minimum, so we are not always waiting on the driver for an image
        uint32_t imageCount = support.capabilities.minImageCount + 1;
        if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) {
            imageCount = support.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = device_->surface()->handle();
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // where one queue family draws and another presents, both of them need to reach the images
        const Device::QueueFamilies& families = device_->families();
        const uint32_t indices[] = { families.graphics, families.present };
        if (families.graphics != families.present) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = indices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = support.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        VkResult result = vkCreateSwapchainKHR(device_->handle(), &createInfo, nullptr, &swapchain_);
        if (result != VK_SUCCESS) {
            swapchain_ = VK_NULL_HANDLE;
            std::stringstream msg;
            msg << "Unable to create the vulkan swapchain - " << resultString(result);
            throw std::runtime_error(msg.str());
        }

        format_ = surfaceFormat.format;
        extent_ = extent;

        uint32_t count = 0;
        result = vkGetSwapchainImagesKHR(device_->handle(), swapchain_, &count, nullptr);
        if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
            std::stringstream msg;
            msg << "Unable to count the vulkan swapchain images - " << resultString(result);
            throw std::runtime_error(msg.str());
        }
        images_.resize(count);
        if (count > 0) {
            result = vkGetSwapchainImagesKHR(device_->handle(), swapchain_, &count, images_.data());
            if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
                std::stringstream msg;
                msg << "Unable to read the vulkan swapchain images - " << resultString(result);
                throw std::runtime_error(msg.str());
            }
        }

        logger_->get()->info("Created a vulkan swapchain of {} images at {} x {}", images_.size(), extent_.width, extent_.height);
    }

    /**
     **/
    void Swapchain::createViews() {
        views_.reserve(images_.size());
        for (VkImage image : images_) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = image;
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = format_;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VkImageView view = VK_NULL_HANDLE;
            VkResult result = vkCreateImageView(device_->handle(), &createInfo, nullptr, &view);
            if (result != VK_SUCCESS) {
                std::stringstream msg;
                msg << "Unable to create a view onto a vulkan swapchain image - " << resultString(result);
                throw std::runtime_error(msg.str());
            }
            views_.push_back(view);
        }
    }

    /**
     **/
    void Swapchain::destroy() {
        for (VkImageView view : views_) {
            vkDestroyImageView(device_->handle(), view, nullptr);
        }
        views_.clear();

        // the images belong to the chain, so they go with it rather than being destroyed
        images_.clear();

        if (swapchain_ != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device_->handle(), swapchain_, nullptr);
            swapchain_ = VK_NULL_HANDLE;
        }
    }

};  // namespace v3d::render::realtime::vulkan

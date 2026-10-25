/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Swapchain.h"

#include <api/log/Logger.h>
#include <api/render/realtime/Present.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/device/Result.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace v3d::render::realtime::vulkan::frame {

namespace {

/**
 * @return how a present mode reads in the log
 **/
const char* describe(Present present) noexcept {
    switch (present) {
    case Present::Uncapped:
        return "uncapped (mailbox)";
    case Present::Vsync:
        return "with vsync (fifo)";
    case Present::AdaptiveVsync:
        return "with adaptive vsync (fifo relaxed)";
    case Present::Immediate:
        return "immediately (immediate)";
    }
    return "";
}

};  // namespace

/**
 **/
Swapchain::Swapchain(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<device::Device>& device, uint32_t width, uint32_t height,
    VkFormat preferred, Present present) :
    device_(device),
    logger_(logger),
    swapchain_(VK_NULL_HANDLE),
    preferred_(preferred),
    wanted_(present),
    presenting_(Present::Vsync),
    format_(VK_FORMAT_UNDEFINED),
    extent_(),
    copyable_(false) {
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
    // the images cannot be destroyed while a queue still uses them
    const VkResult result = vkDeviceWaitIdle(device_->handle());
    device::check(result, "Unable to wait for the vulkan device before recreating the swapchain");
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
const std::vector<VkImage>& Swapchain::images() const noexcept {
    return images_;
}

/**
 **/
const std::vector<VkImageView>& Swapchain::views() const noexcept {
    return views_;
}

/**
 **/
bool Swapchain::copyable() const noexcept {
    return copyable_;
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
    device::check(result, "Unable to read the vulkan surface capabilities");

    uint32_t formatCount = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &formatCount, nullptr);
    device::check(result, "Unable to count the vulkan surface formats", VK_INCOMPLETE);
    support.formats.resize(formatCount);
    if (formatCount > 0) {
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &formatCount, support.formats.data());
        device::check(result, "Unable to read the vulkan surface formats", VK_INCOMPLETE);
        support.formats.resize(formatCount);
    }

    uint32_t modeCount = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &modeCount, nullptr);
    device::check(result, "Unable to count the vulkan present modes", VK_INCOMPLETE);
    support.presentModes.resize(modeCount);
    if (modeCount > 0) {
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &modeCount, support.presentModes.data());
        device::check(result, "Unable to read the vulkan present modes", VK_INCOMPLETE);
        support.presentModes.resize(modeCount);
    }

    if (support.formats.empty() || support.presentModes.empty()) {
        throw std::runtime_error("The vulkan surface offers no format or no present mode to draw with");
    }

    return support;
}

/**
 **/
VkSurfaceFormatKHR Swapchain::chooseFormat(const std::vector<VkSurfaceFormatKHR>& formats, VkFormat preferred) {
    // the caller's format, where the surface offers it
    if (preferred != VK_FORMAT_UNDEFINED) {
        for (const VkSurfaceFormatKHR& format : formats) {
            if (format.format == preferred && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
    }

    // a UNORM format, not an SRGB one, so the colour a shader writes is the colour that
    // appears. An _SRGB format encodes on write. It would treat every colour in the engine as
    // linear and brighten it
    for (const VkSurfaceFormatKHR& format : formats) {
        if ((format.format == VK_FORMAT_B8G8R8A8_UNORM || format.format == VK_FORMAT_R8G8B8A8_UNORM) &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    return formats.front();
}

/**
 **/
VkPresentModeKHR Swapchain::choosePresentMode(const std::vector<VkPresentModeKHR>& modes, Present wanted) {
    const auto offered = [&modes](VkPresentModeKHR mode) {
        return std::find(modes.begin(), modes.end(), mode) != modes.end();
    };
    // the modes each request will take, best first. Fifo is the only mode every implementation
    // has to offer, so it ends every list
    std::vector<VkPresentModeKHR> order;
    switch (wanted) {
    case Present::Uncapped:
        order = {VK_PRESENT_MODE_MAILBOX_KHR};
        break;
    case Present::Vsync:
        break;
    case Present::AdaptiveVsync:
        order = {VK_PRESENT_MODE_FIFO_RELAXED_KHR};
        break;
    case Present::Immediate:
        order = {VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR};
        break;
    }
    for (const VkPresentModeKHR mode : order) {
        if (offered(mode)) {
            return mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

/**
 **/
Present Swapchain::granted(VkPresentModeKHR mode) noexcept {
    switch (mode) {
    case VK_PRESENT_MODE_MAILBOX_KHR:
        return Present::Uncapped;
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
        return Present::AdaptiveVsync;
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
        return Present::Immediate;
    default:
        return Present::Vsync;
    }
}

/**
 **/
void Swapchain::request(Present present) noexcept {
    wanted_ = present;
}

/**
 **/
Present Swapchain::presenting() const noexcept {
    return presenting_;
}

/**
 **/
VkExtent2D Swapchain::chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) {
    // a surface with a current extent requires the swapchain to match it
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

    const VkSurfaceFormatKHR surfaceFormat = chooseFormat(support.formats, preferred_);
    const VkPresentModeKHR presentMode = choosePresentMode(support.presentModes, wanted_);
    const VkExtent2D extent = chooseExtent(support.capabilities, width, height);

    // a window with no area cannot have a swapchain. Leave it empty until the next recreate()
    if (extent.width == 0 || extent.height == 0) {
        format_ = surfaceFormat.format;
        extent_ = extent;
        copyable_ = false;
        if (!arealess_) {
            logger_->get()->info("Window has no area, leaving the vulkan swapchain empty");
        }
        arealess_ = true;
        return;
    }
    arealess_ = false;

    // one more than the minimum, so acquiring an image does not always wait on the driver
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
    // TRANSFER_SRC lets frame::Capture copy an image out. A surface need not support it, and
    // without it the swapchain is still created
    const bool copyable = (support.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (copyable) {
        createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    // where one queue family draws and another presents, both of them need to reach the images
    const device::Device::QueueFamilies& families = device_->families();
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
        throw device::failure(result, "Unable to create the vulkan swapchain");
    }

    format_ = surfaceFormat.format;
    extent_ = extent;
    copyable_ = copyable;
    presenting_ = granted(presentMode);
    logger_->get()->info("Presenting {}x{} images {}", extent.width, extent.height, describe(presenting_));

    uint32_t count = 0;
    result = vkGetSwapchainImagesKHR(device_->handle(), swapchain_, &count, nullptr);
    device::check(result, "Unable to count the vulkan swapchain images", VK_INCOMPLETE);
    images_.resize(count);
    if (count > 0) {
        result = vkGetSwapchainImagesKHR(device_->handle(), swapchain_, &count, images_.data());
        device::check(result, "Unable to read the vulkan swapchain images", VK_INCOMPLETE);
    }

    logger_->get()->info("Created a vulkan swapchain of {} images at {} x {}", images_.size(), extent_.width, extent_.height);

    // warn only when the caller named a format and did not get it, because the caller's
    // colours may depend on the format
    if (preferred_ != VK_FORMAT_UNDEFINED && format_ != preferred_) {
        logger_->get()->warn("The surface does not offer swapchain format {}, so {} is what the chain was built with",
            static_cast<int>(preferred_), static_cast<int>(format_));
    }
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
        const VkResult result = vkCreateImageView(device_->handle(), &createInfo, nullptr, &view);
        device::check(result, "Unable to create a view onto a vulkan swapchain image");
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

    // the swapchain owns its images and destroys them with itself
    images_.clear();

    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_->handle(), swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
    copyable_ = false;
}

};  // namespace v3d::render::realtime::vulkan::frame

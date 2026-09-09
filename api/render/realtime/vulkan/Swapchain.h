/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Device.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan {

/**
 * The chain of images the device renders into and the surface presents from.
 *
 * A window with no area - a minimized one, typically - has no chain that can be
 * built for it. Rather than fail, the chain is left empty in that case and valid()
 * says so, leaving the caller to skip drawing and try again after the next resize.
 **/
class Swapchain final {
 public:
    /**
     * @param logger
     * @param device the device whose queues will draw to and present the images
     * @param width the width to size the images to, when the surface leaves us the choice
     * @param height the height to size the images to, when the surface leaves us the choice
     **/
    Swapchain(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Device>& device, uint32_t width, uint32_t height);

    /**
     **/
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    /**
     * Throw the chain away and build a new one, sized for a window that has changed.
     * Waits for the device to go idle first, so nothing is still reading the old images.
     **/
    void recreate(uint32_t width, uint32_t height);

    /**
     * @return whether there is a chain to draw into
     **/
    bool valid() const noexcept;

    /**
     * @return the underlying swapchain handle
     **/
    VkSwapchainKHR handle() const noexcept;

    /**
     * @return the format the images were created with
     **/
    VkFormat format() const noexcept;

    /**
     * @return the size of the images
     **/
    const VkExtent2D& extent() const noexcept;

    /**
     * @return each image in the chain - owned by the chain, not by us
     **/
    const std::vector<VkImage>& images() const noexcept;

    /**
     * @return a view onto each image in the chain
     **/
    const std::vector<VkImageView>& views() const noexcept;

    /**
     * @return how many images the chain holds
     **/
    std::size_t length() const noexcept;

 private:
    /**
     * What the surface will let us build, on the device we settled on.
     **/
    struct Support {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    /**
     **/
    Support querySupport() const;

    /**
     * @return a 32 bit srgb format where one is offered, otherwise whatever comes first
     **/
    static VkSurfaceFormatKHR chooseFormat(const std::vector<VkSurfaceFormatKHR>& formats);

    /**
     * @return mailbox where the device offers it, otherwise fifo, which always is
     **/
    static VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes);

    /**
     * @return the extent the surface dictates, or our own size clamped to what it allows
     **/
    static VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);

    /**
     **/
    void create(uint32_t width, uint32_t height);

    /**
     **/
    void createViews();

    /**
     **/
    void destroy();

    boost::shared_ptr<Device> device_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    VkSwapchainKHR swapchain_;
    VkFormat format_;
    VkExtent2D extent_;
    std::vector<VkImage> images_;
    std::vector<VkImageView> views_;
};

};  // namespace v3d::render::realtime::vulkan

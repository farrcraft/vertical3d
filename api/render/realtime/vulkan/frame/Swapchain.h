/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/Present.h>
#include <api/render/realtime/vulkan/device/Device.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 * The swapchain: the images the device renders into and the surface presents.
 *
 * A window with no area, such as a minimized one, cannot have a swapchain. In that case the
 * Swapchain is left empty and valid() returns false. The caller skips drawing and calls
 * recreate() after the next resize.
 *
 * The colour format is UNORM unless the caller names one, so the colour a shader writes is the
 * colour that appears. format() returns the format chosen, and a pipeline that draws into the
 * swapchain is built against it.
 *
 * The present mode is a request in the same way. presenting() returns the mode granted, and
 * each build of the swapchain logs it.
 **/
class Swapchain final {
 public:
    /**
     * @param logger
     * @param device the device whose queues will draw to and present the images
     * @param width the width of the images, when the surface lets the caller choose
     * @param height the height of the images, when the surface lets the caller choose
     * @param preferred the colour format to present through, where the surface offers it.
     *        VK_FORMAT_UNDEFINED, or a format the surface does not offer, selects the UNORM
     *        default. format() returns the format chosen.
     * @param present how the images are to be presented, as choosePresentMode() decides
     **/
    Swapchain(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<device::Device>& device, uint32_t width, uint32_t height,
        VkFormat preferred = VK_FORMAT_UNDEFINED, Present present = Present::Uncapped);

    /**
     **/
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    /**
     * Destroy the swapchain and create a new one at the window's new size. Waits for the
     * device to go idle first, so no queue still uses the old images.
     *
     * The format is chosen again with the same preference, so a pipeline built against it
     * does not have to be rebuilt.
     **/
    void recreate(uint32_t width, uint32_t height);

    /**
     * Ask for a different present mode. It takes effect at the next recreate().
     **/
    void request(Present present) noexcept;

    /**
     * @return the present mode the swapchain was last built with, which is the one granted
     *         rather than the one requested
     **/
    Present presenting() const noexcept;

    /**
     * @return whether there is a swapchain to draw into
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
     * @return each swapchain image. The swapchain owns them, so they are never destroyed
     *         directly.
     **/
    const std::vector<VkImage>& images() const noexcept;

    /**
     * @return a view onto each swapchain image
     **/
    const std::vector<VkImageView>& views() const noexcept;

    /**
     * Whether the images can be copied out of, which frame::Capture requires. The swapchain
     * requests TRANSFER_SRC usage where the surface supports it, and is created without it
     * where the surface does not.
     *
     * @return whether the images were created with TRANSFER_SRC usage
     **/
    bool copyable() const noexcept;

    /**
     * @return how many images the swapchain holds
     **/
    std::size_t length() const noexcept;

    /**
     * Choose the swapchain format from the formats a surface offers.
     *
     * Public because it has no side effects and needs no device, so a test can check it on a
     * machine with no GPU.
     *
     * @param formats what the surface offers, as vulkan reported them
     * @param preferred the caller's choice, or VK_FORMAT_UNDEFINED for none
     * @return the preferred format in a non linear srgb colour space where it is offered,
     *         otherwise a 32 bit UNORM one, otherwise the first offered
     **/
    static VkSurfaceFormatKHR chooseFormat(const std::vector<VkSurfaceFormatKHR>& formats,
        VkFormat preferred = VK_FORMAT_UNDEFINED);

    /**
     * Choose the present mode from the modes a surface offers.
     *
     * Public for the same reason as chooseFormat().
     *
     * @param modes what the surface offers, as vulkan reported them
     * @param wanted the caller's request. Each request falls back as Present describes, and
     *        ends at fifo, which every implementation offers
     **/
    static VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes, Present wanted);

    /**
     * @return the request a vulkan present mode answers, for a mode choosePresentMode() returns
     **/
    static Present granted(VkPresentModeKHR mode) noexcept;

 private:
    /**
     * What the surface supports on the device in use.
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
     * @return the surface's current extent when it has one, otherwise the requested size
     *         clamped to the surface's limits
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

    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    VkSwapchainKHR swapchain_;
    VkFormat preferred_;
    Present wanted_;      /**< the present mode the next build asks for **/
    Present presenting_;  /**< and the one the last build was granted **/
    VkFormat format_;
    VkExtent2D extent_;
    bool copyable_;
    // whether the last create() found no area. The empty swapchain is logged once, not on
    // every recreate while the window has no area
    bool arealess_ = false;
    std::vector<VkImage> images_;
    std::vector<VkImageView> views_;
};

};  // namespace v3d::render::realtime::vulkan::frame

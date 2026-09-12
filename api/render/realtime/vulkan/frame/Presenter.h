/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/vulkan/device/Device.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

#include "Ring.h"
#include "Swapchain.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 * The acquire / record / submit / present loop, and the synchronization it needs.
 *
 * The frames recorded ahead of the device are a Ring, which this drives rather than owns -
 * ADR-0051. What is left here is what needs the chain: an image-available semaphore per
 * frame, and a render-finished semaphore per swapchain image rather than per frame, because
 * it is presentation that waits on it and presentation is tied to the image.
 *
 * The submit made by present() is what signals the ring's fence for the frame it recorded.
 *
 * Nothing here recreates the swapchain - a caller that is told the chain is out of date
 * recreates it and calls reset(), because only the caller knows the window's new size.
 **/
class Presenter final {
 public:
    /**
     * What acquire() left the caller with.
     **/
    enum class Status {
        Ready,       /**< there is an image to draw into **/
        Skip,        /**< there is nothing to draw into and nothing to be done about it - a minimized window **/
        OutOfDate    /**< the chain no longer matches the surface and has to be rebuilt **/
    };

    /**
     * The image a frame was given, and the buffer its commands are recorded into.
     **/
    struct Acquisition {
        Acquisition() noexcept;

        uint32_t image;           /**< index into the swapchain's images and views **/
        VkCommandBuffer commands; /**< begun and ready to record into **/
    };

    /**
     * @param logger
     * @param swapchain the chain images are acquired from
     * @param ring the frames to pace against, whose device is the one that presents
     **/
    Presenter(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Swapchain>& swapchain,
        const boost::shared_ptr<Ring>& ring);

    /**
     **/
    ~Presenter();

    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

    /**
     * Wait for the frame's previous submission to finish, take the next image from the
     * chain, and begin its command buffer.
     * @param acquisition filled in when the status is Ready
     * @throw std::runtime_error if the device fails rather than the chain going stale
     **/
    Status acquire(Acquisition* acquisition);

    /**
     * End the command buffer, submit it, and present the image it drew into.
     * @return OutOfDate when the chain needs rebuilding before the next frame
     * @throw std::runtime_error if the submission fails
     **/
    Status present(const Acquisition& acquisition);

    /**
     * Rebuild what is sized by the swapchain. Call after recreating the chain, since a
     * new chain may hold a different number of images.
     **/
    void reset();

    /**
     * @return the frames this paces against, which is what anything keeping a resource per
     *         frame in flight indexes by
     **/
    boost::shared_ptr<Ring> ring() const noexcept;

 private:
    /**
     **/
    void createSync();

    /**
     **/
    void destroyImageSync();

    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<Swapchain> swapchain_;
    boost::shared_ptr<Ring> ring_;
    std::vector<VkSemaphore> imageAvailable_;
    std::vector<VkSemaphore> renderFinished_;
    bool suboptimal_;
};

};  // namespace v3d::render::realtime::vulkan::frame

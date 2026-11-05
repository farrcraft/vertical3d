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
 * Takes each frame through acquire, record, submit and present, and owns the semaphores that
 * order those steps.
 *
 * Each frame in the Ring has a semaphore that signals when its swapchain image is ready to draw
 * into. Each swapchain image has a semaphore that signals when drawing into it has finished.
 * The second kind is per image because a present waits on it, and a present is for one image.
 * The Ring also works without a Presenter, for drawing offscreen.
 *
 * present() submits the frame's commands. That submit signals the Ring's fence for the frame.
 *
 * Another window's swapchain can be joined to the Presenter. A joined swapchain is acquired,
 * drawn and presented in each of the Presenter's frames, with the same command buffer, one
 * submission and one present. A joined swapchain that has no image to give is left out of that
 * frame, and its own window is drawn without it.
 *
 * The Presenter never recreates the swapchain. When acquire() or present() returns OutOfDate,
 * the caller recreates the swapchain at the window's new size, then calls reset().
 **/
class Presenter final {
 public:
    /**
     * What acquire() left the caller with.
     **/
    enum class Status : std::uint8_t {
        Ready,       /**< there is an image to draw into **/
        Skip,        /**< there is no image to draw into, as for a minimized window **/
        OutOfDate    /**< the swapchain does not match the surface and has to be recreated **/
    };

    /**
     * Another window's swapchain, presented in the Presenter's frames, and the semaphores that
     * order its steps. It keeps a semaphore per frame in flight and one per image, as the
     * Presenter does for its own.
     **/
    class Joined final {
     public:
        /**
         * @throw std::runtime_error if its semaphores cannot be made
         **/
        Joined(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Swapchain>& swapchain, uint32_t framesInFlight);
        ~Joined();

        Joined(const Joined&) = delete;
        Joined& operator=(const Joined&) = delete;

        /**
         * @return the swapchain presented
         **/
        boost::shared_ptr<Swapchain> swapchain() const noexcept;

        /**
         * @return whether the swapchain no longer matches its surface, and has to be recreated
         *         before it is drawn again. A recreate is followed by reset()
         **/
        bool stale() const noexcept;

        /**
         * Recreate the per-image semaphores after the swapchain is recreated.
         **/
        void reset();

     private:
        friend class Presenter;

        void destroyImageSync() noexcept;
        void destroySync() noexcept;

        boost::shared_ptr<device::Device> device_;
        boost::shared_ptr<Swapchain> swapchain_;
        std::vector<VkSemaphore> imageAvailable_;
        std::vector<VkSemaphore> renderFinished_;
        bool stale_;
    };

    /**
     * An image a joined swapchain gave a frame.
     **/
    struct Taken {
        boost::shared_ptr<Joined> joined;
        uint32_t image;  /**< index into that swapchain's images and views **/
    };

    /**
     * The image a frame was given, and the buffer its commands are recorded into.
     **/
    struct Acquisition {
        Acquisition() noexcept;

        uint32_t image;           /**< index into the swapchain's images and views **/
        VkCommandBuffer commands; /**< begun and ready to record into **/
        std::vector<Taken> joined;  /**< the images each joined swapchain that had one gave **/
    };

    /**
     * @param logger
     * @param swapchain the swapchain to acquire images from
     * @param ring the frames in flight. Its device presents.
     **/
    Presenter(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Swapchain>& swapchain,
        const boost::shared_ptr<Ring>& ring);

    /**
     **/
    ~Presenter();

    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

    /**
     * Join another window's swapchain, to be presented in each frame from the next.
     * @throw std::runtime_error if its semaphores cannot be made
     **/
    boost::shared_ptr<Joined> join(const boost::shared_ptr<Swapchain>& swapchain);

    /**
     * Stop presenting a joined swapchain. It waits for the device, so no submission still uses
     * the swapchain's semaphores when they go.
     **/
    void leave(const boost::shared_ptr<Joined>& joined);

    /**
     * Wait for the frame's previous submission to finish, acquire the next swapchain image and
     * an image of each joined swapchain that has one, and begin the frame's command buffer.
     * @param acquisition filled in when the status is Ready
     * @throw std::runtime_error if the device fails. An out-of-date swapchain returns OutOfDate.
     **/
    Status acquire(Acquisition* acquisition);

    /**
     * End the command buffer, submit it, and present the image it drew into and each joined
     * image. A joined swapchain whose present finds it out of date is marked stale.
     * @return OutOfDate when the swapchain must be recreated before the next frame
     * @throw std::runtime_error if the submission fails
     **/
    Status present(const Acquisition& acquisition);

    /**
     * Recreate the per-image semaphores. Call after recreating the swapchain, because the new
     * swapchain may hold a different number of images.
     **/
    void reset();

    /**
     * @return the Ring. Anything that keeps one resource per frame in flight indexes it by
     *         the Ring's frame().
     **/
    boost::shared_ptr<Ring> ring() const noexcept;

 private:
    /**
     **/
    void createSync();

    /**
     **/
    void destroyImageSync() noexcept;

    /**
     * Destroy every semaphore, for the destructor and for a constructor that threw.
     **/
    void destroySync() noexcept;

    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<Swapchain> swapchain_;
    boost::shared_ptr<Ring> ring_;
    std::vector<VkSemaphore> imageAvailable_;
    std::vector<VkSemaphore> renderFinished_;
    std::vector<boost::shared_ptr<Joined>> joined_;
    bool suboptimal_;
};

};  // namespace v3d::render::realtime::vulkan::frame

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <functional>
#include <vector>

#include "CommandPool.h"
#include "Retirement.h"
#include "Timings.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 * The frames recorded ahead of the one the device is still drawing, and what each of them
 * owns: a command buffer and the fence its submission signals.
 *
 * This needs only a device. Pacing frames is separate from presenting. Everything keeping a
 * resource per frame in flight is indexed by the ring, so a renderer works the same whether
 * its frames end up on a screen or in a file.
 *
 * The fence is created here and waited on here, but is signalled by whichever submit the
 * caller makes - Presenter's, or a test's. The caller's submit must signal it, so
 * submitting() is public.
 *
 * Because the ring tracks when a frame has finished, it is also where something released
 * during play waits to be destroyed. Anything driving frames has to begin them through
 * begin(), or nothing retired is ever collected.
 **/
class Ring final {
 public:
    /**
     * @param device the device the buffers are allocated on and the fences live on
     * @param framesInFlight how many frames may be recorded ahead of the device, which is
     *        clamped up to one because a ring of none can record nothing
     * @throw std::runtime_error if a fence cannot be created
     **/
    explicit Ring(const boost::shared_ptr<device::Device>& device, uint32_t framesInFlight = 2);

    /**
     **/
    ~Ring();

    Ring(const Ring&) = delete;
    Ring& operator=(const Ring&) = delete;

    /**
     * @return the device the ring was built on
     **/
    boost::shared_ptr<device::Device> device() const noexcept;

    /**
     * @return how many frames may be recorded ahead of the device
     **/
    uint32_t framesInFlight() const noexcept;

    /**
     * @return which of those frames is being recorded, and so which slot of any per-frame
     *         resource the caller keeps is the one to write into
     **/
    uint32_t frame() const noexcept;

    /**
     * Wait until the frame that will be recorded next has finished its last submission.
     *
     * begin() does this itself. It is exposed for anything else keeping a resource per frame
     * in flight - a geometry buffer a batcher rewrites, typically - which has to write into
     * that slot before the frame is recorded, while the device may still be reading what was
     * in it two frames ago.
     *
     * @throw std::runtime_error if the wait fails
     **/
    void waitFrame() const;

    /**
     * Wait until the device has finished everything that was submitted to it.
     * @throw std::runtime_error if the wait fails
     **/
    void waitIdle() const;

    /**
     * waitIdle() for a destructor or a teardown, which must not throw. A failed wait is
     * ignored, because the caller has no way to report it.
     **/
    void waitIdleNoThrow() const noexcept;

    /**
     * @return how many frames have been begun since the ring was built
     **/
    uint64_t begun() const noexcept;

    /**
     * Record a frame that ended without being begun, because there was no image to draw
     * into. Per-frame state that is reset when a frame begins is reset by this too, through
     * turns().
     **/
    void skip() noexcept;

    /**
     * @return a count that changes every time a frame is begun, begun again after being
     *         abandoned, or skipped. Per-frame state that restarts when a frame begins
     *         compares against it. Unlike begun(), it counts a slot begun again, because the
     *         state recorded for the abandoned attempt was never drawn.
     **/
    uint64_t turns() const noexcept;

    /**
     * @return the count, as begun() counts, of the frame a draw item queued now is recorded
     *         into. That is the frame begun and not yet submitted, or else the next one to be
     *         begun.
     **/
    uint64_t recording() const noexcept;

    /**
     * Hold a destruction back until every frame that may name the released object has
     * finished.
     *
     * The last frame that may name it is recording(), because draw items queued for that frame
     * before the release may name the object. The callback runs from a later begin(), or from
     * the destructor once the device is idle, and is the last use of whatever it captured.
     **/
    void retire(std::function<void()> destroy);

    /**
     * Unsignal the current frame's fence and return it, for the submit that ends the frame to
     * signal. Called immediately before that submit and nowhere else: a frame that is begun and
     * then abandoned, because recording threw, leaves the fence signalled, so nothing that
     * waits on it later waits forever.
     *
     * @throw std::runtime_error if the fence cannot be reset
     **/
    VkFence submitting();

    /**
     * Wait for the current frame's last submission and begin its command buffer.
     *
     * The fence stays signalled until submitting(), so a caller that gives up at any point
     * before the submit leaves the ring able to begin the frame again. A frame begun again
     * this way is not counted a second time. Once a new frame is begun, whatever was retired
     * framesInFlight frames ago is destroyed.
     *
     * Only the ring recovers this way. A swapchain image already acquired for the abandoned
     * frame, and the semaphore its acquire signalled, are not given back, so a presenting app
     * that catches a recording failure cannot keep drawing.
     *
     * @return the buffer to record into
     * @throw std::runtime_error if the fence or the buffer cannot be made ready
     **/
    VkCommandBuffer begin();

    /**
     * Move to the next frame. Call after the submit, since everything indexed by frame()
     * refers to the one just recorded until then.
     **/
    void advance() noexcept;

    /**
     * How long the device spent on what each slot recorded, read when the slot is begun
     * again. The recorder times every pass of a frame it records, and a caller recording
     * its own commands into begin()'s buffer may open and close spans of its own.
     **/
    Timings& timings() noexcept;

 private:
    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<CommandPool> pool_;
    std::vector<VkCommandBuffer> commands_;
    std::vector<VkFence> inFlight_;
    uint32_t framesInFlight_;
    uint32_t frame_;
    uint64_t begun_;
    uint64_t skipped_ = 0;
    uint64_t starts_ = 0;   /**< every successful begin(), a slot begun again included **/
    bool pending_ = false;  /**< whether the current slot was begun and not yet submitted **/
    Retirement retired_;
    boost::shared_ptr<Timings> timings_;
};

};  // namespace v3d::render::realtime::vulkan::frame

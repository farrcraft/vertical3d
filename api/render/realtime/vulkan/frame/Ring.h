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
 * The frames in flight: frames recorded while the device still draws earlier ones. Each frame
 * owns a command buffer and a fence that its submit signals.
 *
 * A Ring needs only a device, not a swapchain. Every resource kept per frame in flight is
 * indexed by the Ring's frame(), so a renderer works the same whether its frames go to a
 * screen or to a file.
 *
 * The Ring creates each fence and waits on it. The caller's submit signals it, whether that
 * submit is Presenter's or a test's, so submitting() is public.
 *
 * The Ring tracks when each frame finishes, so it also holds objects released during play
 * until no frame can read them. Every frame has to start with begin(), because begin() is
 * where retired objects are destroyed.
 **/
class Ring final {
 public:
    /**
     * @param device the device the buffers are allocated on and the fences live on
     * @param framesInFlight how many frames may be recorded ahead of the device. A value of
     *        zero is raised to one.
     * @throw std::runtime_error if a fence cannot be created
     **/
    explicit Ring(const boost::shared_ptr<device::Device>& device, uint32_t framesInFlight = 2);

    /**
     **/
    ~Ring();

    Ring(const Ring&) = delete;
    Ring& operator=(const Ring&) = delete;

    /**
     * @return the device the Ring was built on
     **/
    boost::shared_ptr<device::Device> device() const noexcept;

    /**
     * @return how many frames may be recorded ahead of the device
     **/
    uint32_t framesInFlight() const noexcept;

    /**
     * @return the index of the frame being recorded. A caller that keeps one resource per
     *         frame in flight writes into the slot at this index.
     **/
    uint32_t frame() const noexcept;

    /**
     * Wait until the frame that will be recorded next has finished its last submission.
     *
     * begin() also waits. waitFrame() is for a per-frame resource that is written before the
     * frame begins, such as a StreamRing buffer. Until the wait returns, the device may still
     * read what that slot held framesInFlight frames ago.
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
     * @return how many frames have been begun since the Ring was built
     **/
    uint64_t begun() const noexcept;

    /**
     * Count a frame that was never begun because there was no image to draw into. skip()
     * changes turns(), so per-frame state that resets when a frame begins also resets here.
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
     * @return the begun() count of the frame that a draw item queued now is recorded into:
     *         the frame begun and not yet submitted, or else the next one to be begun.
     **/
    uint64_t recording() const noexcept;

    /**
     * Delay a destruction until every frame that may name the released object has finished.
     *
     * The last frame that may name it is recording(), because draw items queued for that frame
     * before the release may name the object. destroy runs from a later begin(), or from the
     * destructor once the device is idle. It is the last use of whatever it captured.
     **/
    void retire(std::function<void()> destroy);

    /**
     * Reset the current frame's fence and return it, for the submit that ends the frame to
     * signal. Call it immediately before that submit and nowhere else. A frame that is begun
     * and then abandoned, because recording threw, then leaves the fence signalled, and a later
     * wait on it does not block forever.
     *
     * @throw std::runtime_error if the fence cannot be reset
     **/
    VkFence submitting();

    /**
     * Wait for the current frame's last submission and begin its command buffer.
     *
     * The fence stays signalled until submitting(), so a caller that gives up at any point
     * before the submit leaves the Ring able to begin the frame again. A frame begun again
     * this way is not counted a second time. Once a new frame is begun, whatever was retired
     * framesInFlight frames ago is destroyed.
     *
     * Only the Ring recovers this way. A swapchain image already acquired for the abandoned
     * frame, and the semaphore its acquire signalled, are not given back, so a presenting app
     * that catches a recording failure cannot keep drawing.
     *
     * @return the buffer to record into
     * @throw std::runtime_error if the fence or the buffer cannot be made ready
     **/
    VkCommandBuffer begin();

    /**
     * Move to the next frame. Call after the submit, because until then every per-frame
     * resource indexed by frame() belongs to the frame just recorded.
     **/
    void advance() noexcept;

    /**
     * The device time taken by each slot's commands, read back when the slot is begun again.
     * Recorder times every pass of a frame it records. A caller that records its own commands
     * into begin()'s buffer may open and close its own spans.
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

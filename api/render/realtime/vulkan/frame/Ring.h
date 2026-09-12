/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

#include "CommandPool.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 * The frames recorded ahead of the one the device is still drawing, and what each of them
 * owns: a command buffer and the fence its submission signals.
 *
 * This needs a device and nothing else - ADR-0051. Pacing the device is not presenting, and a
 * ring is what everything keeping a resource per frame in flight is actually indexed by, so a
 * renderer works the same whether the frames it paces end up on a screen or in a file.
 *
 * The fence is created here and waited on here, and is signalled by whichever submit the
 * caller makes - Presenter's, or a test's. That is the one thing about a ring a reader has to
 * be told rather than infer, and it is why fence() is exposed at all.
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
     **/
    void waitIdle() const;

    /**
     * The fence the current frame's submit has to signal, and that waitFrame() waits on. A
     * submit that does not signal it leaves the next turn around the ring waiting forever.
     **/
    VkFence fence() const noexcept;

    /**
     * Wait for the current frame's last submission, unsignal its fence and begin its command
     * buffer.
     *
     * The fence is only reset once the frame is going to be submitted, so a caller that gives
     * up between waitFrame() and here leaves the ring as it found it.
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

 private:
    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<CommandPool> pool_;
    std::vector<VkCommandBuffer> commands_;
    std::vector<VkFence> inFlight_;
    uint32_t framesInFlight_;
    uint32_t frame_;
};

};  // namespace v3d::render::realtime::vulkan::frame

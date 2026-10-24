/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/device/Instance.h>

#include <vulkan/vulkan.h>

#include <boost/shared_ptr.hpp>

namespace v3d::test {

/**
 * What main() returns when there is no device to draw with. The test's SKIP_RETURN_CODE is 77,
 * so ctest reports a skip, not a failure. 77 is the automake convention, which ctest follows.
 **/
const int skipExitCode = 77;

/**
 * @return whether an instance and a device can be created at all, so that a runner with no
 *         gpu can be told apart from a broken one
 **/
bool deviceAvailable();

/**
 * An instance, a surface-free device and a context to draw with, for a case that needs all
 * three but does not test any of them.
 *
 * The context receives its target format at construction, because every pipeline it compiles
 * is built against that format. If the format were set afterwards, the renderers would already
 * have been built against VK_FORMAT_UNDEFINED.
 *
 * Each case constructs its own, so state one case leaves on the device cannot affect another.
 * This creates a device per case, and the cases fail independently.
 **/
struct Headless {
    /**
     * @param colour the format of what the cases will draw into
     * @param width in pixels
     * @param height in pixels
     * @param allocations how the device finds memory, for a case that asserts on the
     *        allocator rather than on what was drawn
     * @throw std::runtime_error if there is no device, which main() has already ruled out
     **/
    Headless(VkFormat colour, uint32_t width, uint32_t height,
        render::realtime::vulkan::memory::Allocator::Kind allocations =
            render::realtime::vulkan::memory::Allocator::Kind::Direct);

    ~Headless();

    Headless(const Headless&) = delete;
    Headless& operator=(const Headless&) = delete;

    /**
     * Submit a command buffer from the context's ring and wait for it to finish.
     *
     * The submission signals the ring slot's fence, so the next use of that slot waits on this
     * submission as it would on a presented frame.
     *
     * @param commands a buffer from ring()->begin(), still recording
     **/
    void submitAndWait(VkCommandBuffer commands);

    /**
     * Submit a command buffer from the context's ring without waiting, as a presented frame is
     * submitted. The work is still in flight when the case continues.
     *
     * @param commands a buffer from ring()->begin(), still recording
     **/
    void submit(VkCommandBuffer commands);

    /**
     * @return whether the validation layer was on and reported no errors and no warnings
     **/
    bool silent() const;

    boost::shared_ptr<v3d::log::Logger> logger;
    boost::shared_ptr<render::realtime::vulkan::device::Instance> instance;
    boost::shared_ptr<render::realtime::vulkan::device::Device> device;
    boost::shared_ptr<render::realtime::DeviceContext> context;
};

};  // namespace v3d::test

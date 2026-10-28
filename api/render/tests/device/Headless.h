/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/RenderTarget.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/device/Instance.h>
#include <api/render/realtime/vulkan/frame/Capture.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>

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
            render::realtime::vulkan::memory::Allocator::Kind::Suballocated);

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
     * Which image of a render target draw() copies out.
     **/
    enum class Read : uint8_t {
        Colour,  /**< the colour image, which the frame leaves in SHADER_READ_ONLY_OPTIMAL **/
        Depth    /**< the depth image, which has to be sampled, and is left in DEPTH_READ_ONLY_OPTIMAL **/
    };

    /**
     * Record a frame, copy a render target out of it, submit, and wait.
     *
     * Every pass of the frame names its own target, because the frame is given no image of
     * its own. Once this returns, the capture holds what the frame left in the target, for
     * write(), depth() or checkReference().
     *
     * @param frame what to draw, with the context's frame uniforms
     * @param target one of the targets the frame's passes draw into
     * @param capture where the copy goes
     * @param read which of the target's images is copied
     **/
    void draw(const render::realtime::Frame& frame, const render::realtime::RenderTarget& target,
        render::realtime::vulkan::frame::Capture* capture, Read read = Read::Colour);

    /**
     * draw() a frame, write the target's colour to a png, and read the png back.
     *
     * The picture is read from the file, so a case that checks texels by hand reads the same
     * bytes a reference comparison does, and the file is left for a person to look at.
     *
     * @param path where the png is written
     * @return the picture, or null when the png cannot be written or read
     **/
    boost::shared_ptr<image::Image> picture(const render::realtime::Frame& frame, const render::realtime::RenderTarget& target,
        const std::string& path);

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

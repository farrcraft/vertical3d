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
 * What main() returns when there is no device to draw with. ctest is told to read it as a skip
 * rather than a failure; 77 is the convention automake set and ctest inherited.
 **/
const int skipExitCode = 77;

/**
 * @return whether an instance and a device can be had at all, which is what separates a runner
 *         with no gpu from a broken one
 **/
bool deviceAvailable();

/**
 * An instance, a surface-free device and a context to draw with, for a case that needs all
 * three and cares about none of them.
 *
 * The context is built knowing what it draws into, because every pipeline it compiles is built
 * against that format - a fixture that described itself afterwards would build its renderers
 * against VK_FORMAT_UNDEFINED.
 *
 * Each case gets its own, so what one leaves on the device cannot reach another. That costs a
 * device creation per case, which is the price of cases that fail independently.
 **/
struct Headless {
    /**
     * @param colour the format of what the cases will draw into
     * @param width in pixels
     * @param height in pixels
     * @throw std::runtime_error if there is no device, which main() has already ruled out
     **/
    Headless(VkFormat colour, uint32_t width, uint32_t height);

    ~Headless();

    Headless(const Headless&) = delete;
    Headless& operator=(const Headless&) = delete;

    /**
     * Submit a command buffer from the context's ring and wait for it to finish.
     *
     * The ring's fence is what it is signalled with, so the next turn around the ring waits on
     * this submission the way a presented frame's would - ADR-0051.
     *
     * @param commands a buffer from ring()->begin(), still recording
     **/
    void submitAndWait(VkCommandBuffer commands);

    /**
     * @return whether the validation layer reported nothing, having been on to report it
     **/
    bool silent() const;

    boost::shared_ptr<v3d::log::Logger> logger;
    boost::shared_ptr<render::realtime::vulkan::device::Instance> instance;
    boost::shared_ptr<render::realtime::vulkan::device::Device> device;
    boost::shared_ptr<render::realtime::DeviceContext> context;
};

};  // namespace v3d::test

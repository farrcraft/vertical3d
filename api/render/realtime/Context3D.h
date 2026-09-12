/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/Presenter.h>
#include <api/render/realtime/vulkan/frame/Swapchain.h>

#include "DeviceContext.h"
#include "Window.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
/**
 * The 3D render context - a DeviceContext whose destination is a window: the chain of images
 * presented to it, and the loop that gets one onto the screen.
 *
 * What the base owns is everything that only needs a device. What is here is what needs the
 * surface, which is the chain, the presenter, and keeping the base's description of its
 * destination in step with a chain that is rebuilt whenever the window changes size.
 **/
class Context3D : public DeviceContext {
 public:
    /**
     * @param logger
     * @param window the window the context renders to
     * @param preferred the colour format to present through - Swapchain, ADR-0049. What
     *        was settled on is swapchain()->format(), which is what the quad renderer and
     *        every other pipeline drawing into the chain is built against.
     **/
    Context3D(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Window>& window,
        VkFormat preferred = VK_FORMAT_UNDEFINED);

    /**
     **/
    ~Context3D() override;

    /**
     * @return the chain of images being presented to the window
     **/
    boost::shared_ptr<vulkan::frame::Swapchain> swapchain() const;

    /**
     * @return the acquire, submit and present loop the frames go through
     **/
    boost::shared_ptr<vulkan::frame::Presenter> presenter() const;

    /**
     * Rebuild the swapchain against the window's current size, and everything that is
     * sized by it. Call this when presenting reports the chain has gone out of date.
     **/
    void resize();

 private:
    /**
     * The device the base is built on, taken from the window's instance and surface.
     *
     * Static because it runs in the member initializer list, before there is an object: the
     * base class needs a device and the only place one can come from is the window.
     *
     * @throw std::runtime_error if the window has not been created
     **/
    static boost::shared_ptr<vulkan::device::Device> deviceFor(const boost::shared_ptr<v3d::log::Logger>& logger,
        const boost::shared_ptr<Window>& window);

    boost::shared_ptr<Window> window_;
    boost::shared_ptr<vulkan::frame::Swapchain> swapchain_;
    // last, so that it is torn down first - nothing else may go away while a frame it
    // submitted is still in flight
    boost::shared_ptr<vulkan::frame::Presenter> presenter_;
};
};  // namespace v3d::render::realtime

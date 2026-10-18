/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/Presenter.h>
#include <api/render/realtime/vulkan/frame/Swapchain.h>
#include <api/render/realtime/vulkan/memory/Allocator.h>

#include "DeviceContext.h"
#include "Window.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
/**
 * The 3D render context: a DeviceContext that draws into a window.
 *
 * DeviceContext owns everything that needs only a device. Context3D owns what needs the
 * surface: the swapchain and the Presenter. It also keeps DeviceContext's description of the
 * destination in step with the swapchain, which is recreated whenever the window changes size.
 **/
class Context3D : public DeviceContext {
 public:
    /**
     * @param logger
     * @param window the window the context renders to
     * @param preferred the colour format to present through, as Swapchain describes. The
     *        format chosen is swapchain()->format(), which every pipeline drawing into the
     *        swapchain is built against.
     * @param allocations how the device finds memory for its buffers and images
     **/
    Context3D(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Window>& window,
        VkFormat preferred = VK_FORMAT_UNDEFINED,
        vulkan::memory::Allocator::Kind allocations = vulkan::memory::Allocator::Kind::Direct);

    /**
     **/
    ~Context3D() override;

    /**
     * @return the swapchain presented to the window
     **/
    boost::shared_ptr<vulkan::frame::Swapchain> swapchain() const;

    /**
     * @return the Presenter that acquires, submits and presents each frame
     **/
    boost::shared_ptr<vulkan::frame::Presenter> presenter() const;

    /**
     * Recreate the swapchain at the window's current size, and everything sized by it. Call
     * it when the Presenter returns OutOfDate.
     **/
    void resize();

 private:
    /**
     * The device the base is built on, taken from the window's instance and surface.
     *
     * Static because it runs in the member initializer list, before the object exists. The
     * base class needs a device, and only the window can supply one.
     *
     * @throw std::runtime_error if the window has not been created
     **/
    static boost::shared_ptr<vulkan::device::Device> deviceFor(const boost::shared_ptr<v3d::log::Logger>& logger,
        const boost::shared_ptr<Window>& window, vulkan::memory::Allocator::Kind allocations);

    boost::shared_ptr<Window> window_;
    boost::shared_ptr<vulkan::frame::Swapchain> swapchain_;
    // declared last, so it is destroyed first. Its destructor waits for the device, so
    // nothing else is destroyed while a frame it submitted is in flight
    boost::shared_ptr<vulkan::frame::Presenter> presenter_;
};
};  // namespace v3d::render::realtime

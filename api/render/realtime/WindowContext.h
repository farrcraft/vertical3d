/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/frame/Present.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/Presenter.h>
#include <api/render/realtime/vulkan/frame/Swapchain.h>
#include <api/render/realtime/vulkan/memory/Allocator.h>
#include <api/window/Window.h>

#include "DeviceContext.h"

#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
/**
 * The 3D render context: a DeviceContext that draws into a window.
 *
 * DeviceContext owns everything that needs only a device. WindowContext owns what needs the
 * surface: the swapchain and the Presenter. It also keeps DeviceContext's description of the
 * destination in step with the swapchain, which is recreated whenever the window changes size.
 **/
class WindowContext : public DeviceContext {
 public:
    /**
     * Another window the context presents to beside its own. It has a surface and a swapchain
     * of its own, in the format of the context's, and is presented in the context's frames.
     **/
    class Attached final {
     public:
        /**
         * @return the window presented to
         **/
        boost::shared_ptr<v3d::window::Window> window() const noexcept;

        /**
         * @return its swapchain
         **/
        boost::shared_ptr<vulkan::frame::Swapchain> swapchain() const noexcept;

     private:
        friend class WindowContext;

        boost::shared_ptr<v3d::window::Window> window_;
        boost::shared_ptr<vulkan::frame::Swapchain> swapchain_;
        boost::shared_ptr<vulkan::frame::Presenter::Joined> joined_;
    };

    /**
     * @param logger
     * @param window the window the context renders to
     * @param preferred the colour format to present through, as Swapchain describes. The
     *        format chosen is swapchain()->format(), which every pipeline drawing into the
     *        swapchain is built against.
     * @param allocations how the device finds memory for its buffers and images
     * @param present how the swapchain's images are presented, as Swapchain describes
     **/
    WindowContext(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::window::Window>& window,
        VkFormat preferred = VK_FORMAT_UNDEFINED,
        vulkan::memory::Allocator::Kind allocations = vulkan::memory::Allocator::Kind::Suballocated,
        frame::Present present = frame::Present::Uncapped);

    /**
     **/
    ~WindowContext() override;

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

    /**
     * Ask for a different present mode, and recreate the swapchain to take it, as resize()
     * does. swapchain()->presenting() then says which mode was granted.
     **/
    void present(frame::Present present);

    /**
     * Present to another window too, from the next frame.
     *
     * @return the window's hold on the context, or null when the device cannot present to its
     *         surface or the surface does not offer the context's format
     * @throw std::runtime_error if the window has not been created, or the device fails
     **/
    boost::shared_ptr<Attached> attach(const boost::shared_ptr<v3d::window::Window>& window);

    /**
     * Stop presenting to another window. It waits for the device, so the window can be
     * destroyed once this returns.
     **/
    void detach(const boost::shared_ptr<Attached>& attached);

    /**
     * Recreate another window's swapchain at its current size when it no longer matches its
     * surface, or when it had no area and the window has one again.
     **/
    void refresh(const boost::shared_ptr<Attached>& attached);

 private:
    /**
     * The device the base is built on: the process's instance, and a surface made for the
     * window against it. The device holds the surface, so the context has to be destroyed
     * before the window is.
     *
     * Static because it runs in the member initializer list, before the object exists. The
     * base class needs a device, and only the window can supply one.
     *
     * @throw std::runtime_error if the window has not been created
     **/
    static boost::shared_ptr<vulkan::device::Device> deviceFor(const boost::shared_ptr<v3d::log::Logger>& logger,
        const boost::shared_ptr<v3d::window::Window>& window, vulkan::memory::Allocator::Kind allocations);

    boost::shared_ptr<v3d::window::Window> window_;
    boost::shared_ptr<vulkan::frame::Swapchain> swapchain_;
    // declared last, so it is destroyed first. Its destructor waits for the device, so
    // nothing else is destroyed while a frame it submitted is in flight
    boost::shared_ptr<vulkan::frame::Presenter> presenter_;
    // declared after the presenter, so each is let go of before the presenter goes
    std::vector<boost::shared_ptr<Attached>> attached_;
};
};  // namespace v3d::render::realtime

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "WindowContext.h"

#include <api/log/Logger.h>
#include <api/render/realtime/Present.h>
#include <api/render/realtime/SharedInstance.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/device/Instance.h>
#include <api/render/realtime/vulkan/device/Surface.h>
#include <api/render/realtime/vulkan/frame/Presenter.h>
#include <api/render/realtime/vulkan/frame/Swapchain.h>
#include <api/render/realtime/vulkan/memory/Allocator.h>
#include <api/window/Window.h>

#include "DeviceContext.h"

#include <cstdint>
#include <stdexcept>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {
/**
 **/
boost::shared_ptr<vulkan::device::Device> WindowContext::deviceFor(const boost::shared_ptr<v3d::log::Logger>& logger,
    const boost::shared_ptr<v3d::window::Window>& window, vulkan::memory::Allocator::Kind allocations) {
    if (!window || window->sdl() == nullptr) {
        throw std::runtime_error("A window context needs a created window to render to");
    }
    const boost::shared_ptr<vulkan::device::Instance> instance = sharedInstance(logger);
    const boost::shared_ptr<vulkan::device::Surface> surface = boost::make_shared<vulkan::device::Surface>(instance, window->sdl());
    return boost::make_shared<vulkan::device::Device>(logger, instance, surface, allocations);
}

/**
 **/
WindowContext::WindowContext(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::window::Window>& window,
    VkFormat preferred, vulkan::memory::Allocator::Kind allocations, Present present) :
    DeviceContext(logger, deviceFor(logger, window, allocations)),
    window_(window) {
    swapchain_ = boost::make_shared<vulkan::frame::Swapchain>(logger, device(), static_cast<uint32_t>(window_->width()),
        static_cast<uint32_t>(window_->height()), preferred, presentModes(present));
    presenter_ = boost::make_shared<vulkan::frame::Presenter>(logger, swapchain_, ring());
    describe(swapchain_->format(), swapchain_->extent());
}

/**
 **/
WindowContext::~WindowContext() {
    // the device may still be drawing with everything about to be destroyed. The base waits
    // as well, and by then this has already let go of the presenter
    ring()->waitIdleNoThrow();
}

/**
 **/
boost::shared_ptr<vulkan::frame::Swapchain> WindowContext::swapchain() const {
    return swapchain_;
}

/**
 **/
boost::shared_ptr<vulkan::frame::Presenter> WindowContext::presenter() const {
    return presenter_;
}

/**
 **/
void WindowContext::resize() {
    swapchain_->recreate(static_cast<uint32_t>(window_->width()), static_cast<uint32_t>(window_->height()));
    // a new swapchain can hold a different number of images, and the Presenter keeps a
    // semaphore per image
    presenter_->reset();
    // the swapchain's extent, not the window's. The surface may require an extent other than
    // the one requested, and everything sized against the swapchain has to match it
    describe(swapchain_->format(), swapchain_->extent());
}

/**
 **/
void WindowContext::present(Present present) {
    swapchain_->request(presentModes(present));
    resize();
}

};  // namespace v3d::render::realtime

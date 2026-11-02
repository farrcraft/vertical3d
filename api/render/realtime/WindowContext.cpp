/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "WindowContext.h"

#include <api/log/Logger.h>
#include <api/render/realtime/frame/Present.h>
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
#include <vector>

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
    VkFormat preferred, vulkan::memory::Allocator::Kind allocations, frame::Present present) :
    DeviceContext(logger, deviceFor(logger, window, allocations)),
    window_(window) {
    swapchain_ = boost::make_shared<vulkan::frame::Swapchain>(logger, device(), static_cast<uint32_t>(window_->width()),
        static_cast<uint32_t>(window_->height()), preferred, frame::presentModes(present));
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
void WindowContext::present(frame::Present present) {
    swapchain_->request(frame::presentModes(present));
    resize();
}

/**
 **/
boost::shared_ptr<v3d::window::Window> WindowContext::Attached::window() const noexcept {
    return window_;
}

/**
 **/
boost::shared_ptr<vulkan::frame::Swapchain> WindowContext::Attached::swapchain() const noexcept {
    return swapchain_;
}

/**
 **/
boost::shared_ptr<WindowContext::Attached> WindowContext::attach(const boost::shared_ptr<v3d::window::Window>& window) {
    if (!window || window->sdl() == nullptr) {
        throw std::runtime_error("A window is presented to only once it has been created");
    }
    const boost::shared_ptr<vulkan::device::Surface> surface =
        boost::make_shared<vulkan::device::Surface>(sharedInstance(logger()), window->sdl());
    if (!device()->presents(*surface)) {
        logger()->get()->error("The device cannot present to another window's surface");
        return nullptr;
    }
    // every pipeline is built against the context's format, so the window has to take it
    const boost::shared_ptr<vulkan::frame::Swapchain> swapchain = boost::make_shared<vulkan::frame::Swapchain>(logger(), device(),
        static_cast<uint32_t>(window->width()), static_cast<uint32_t>(window->height()), swapchain_->format(),
        std::vector<VkPresentModeKHR>{ swapchain_->presenting() }, surface);
    if (swapchain->valid() && swapchain->format() != swapchain_->format()) {
        logger()->get()->error("Another window's surface does not offer the format the context draws in");
        return nullptr;
    }
    const boost::shared_ptr<Attached> attached = boost::make_shared<Attached>();
    attached->window_ = window;
    attached->swapchain_ = swapchain;
    attached->joined_ = presenter_->join(swapchain);
    attached_.push_back(attached);
    return attached;
}

/**
 **/
void WindowContext::detach(const boost::shared_ptr<Attached>& attached) {
    if (!attached) {
        return;
    }
    presenter_->leave(attached->joined_);
    std::erase(attached_, attached);
}

/**
 **/
void WindowContext::refresh(const boost::shared_ptr<Attached>& attached) {
    const int width = attached->window_->width();
    const int height = attached->window_->height();
    const bool regained = !attached->swapchain_->valid() && width > 0 && height > 0;
    if (!attached->joined_->stale() && !regained) {
        return;
    }
    // a swapchain is recreated only once the device has finished with its images
    ring()->waitIdle();
    attached->swapchain_->recreate(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    attached->joined_->reset();
}

};  // namespace v3d::render::realtime

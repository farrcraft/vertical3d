/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Context3D.h"

#include <api/log/Logger.h>
#include <api/render/realtime/Present.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/Presenter.h>
#include <api/render/realtime/vulkan/frame/Swapchain.h>
#include <api/render/realtime/vulkan/memory/Allocator.h>

#include "DeviceContext.h"
#include "Window.h"

#include <cstdint>
#include <stdexcept>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {
/**
 **/
boost::shared_ptr<vulkan::device::Device> Context3D::deviceFor(const boost::shared_ptr<v3d::log::Logger>& logger,
    const boost::shared_ptr<Window>& window, vulkan::memory::Allocator::Kind allocations) {
    if (!window || !window->instance() || !window->surface()) {
        throw std::runtime_error("A 3D context needs a created window to render to");
    }
    return boost::make_shared<vulkan::device::Device>(logger, window->instance(), window->surface(), allocations);
}

/**
 **/
Context3D::Context3D(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Window>& window,
    VkFormat preferred, vulkan::memory::Allocator::Kind allocations, Present present) :
    DeviceContext(logger, deviceFor(logger, window, allocations)),
    window_(window) {
    swapchain_ = boost::make_shared<vulkan::frame::Swapchain>(logger, device(), static_cast<uint32_t>(window_->width()),
        static_cast<uint32_t>(window_->height()), preferred, present);
    presenter_ = boost::make_shared<vulkan::frame::Presenter>(logger, swapchain_, ring());
    describe(swapchain_->format(), swapchain_->extent());
}

/**
 **/
Context3D::~Context3D() {
    // the device may still be drawing with everything about to be destroyed. The base waits
    // as well, and by then this has already let go of the presenter
    ring()->waitIdleNoThrow();
}

/**
 **/
boost::shared_ptr<vulkan::frame::Swapchain> Context3D::swapchain() const {
    return swapchain_;
}

/**
 **/
boost::shared_ptr<vulkan::frame::Presenter> Context3D::presenter() const {
    return presenter_;
}

/**
 **/
void Context3D::resize() {
    swapchain_->recreate(static_cast<uint32_t>(window_->width()), static_cast<uint32_t>(window_->height()));
    // a new swapchain can hold a different number of images, and the Presenter keeps a
    // semaphore per image
    presenter_->reset();
    // the swapchain's extent, not the window's. The surface may require an extent other than
    // the one requested, and everything sized against the swapchain has to match it
    describe(swapchain_->format(), swapchain_->extent());
    if (hasDepth()) {
        // the depth buffer is only correct while it is the size of what it is attached to
        depth()->recreate(swapchain_->extent().width, swapchain_->extent().height);
    }
}

/**
 **/
void Context3D::present(Present present) {
    swapchain_->request(present);
    resize();
}

};  // namespace v3d::render::realtime

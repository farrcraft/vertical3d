/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Context3D.h"

#include <cstdint>
#include <stdexcept>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {
/**
 **/
Context3D::Context3D(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Window>& window,
    VkFormat preferred) :
    logger_(logger),
    window_(window),
    depthFormat_(VK_FORMAT_UNDEFINED) {
    if (!window_ || !window_->instance() || !window_->surface()) {
        throw std::runtime_error("A 3D context needs a created window to render to");
    }
    device_ = boost::make_shared<vulkan::device::Device>(logger, window_->instance(), window_->surface());
    swapchain_ = boost::make_shared<vulkan::frame::Swapchain>(logger, device_, static_cast<uint32_t>(window_->width()), static_cast<uint32_t>(window_->height()),
        preferred);
    pipelineCache_ = boost::make_shared<vulkan::pipeline::Cache>(device_);
    resources_ = boost::make_shared<vulkan::pipeline::Resources>(device_);
    presenter_ = boost::make_shared<vulkan::frame::Presenter>(logger, device_, swapchain_);
    uploader_ = boost::make_shared<vulkan::memory::Uploader>(device_);
    frameUniforms_ = boost::make_shared<vulkan::frame::FrameUniforms>(device_, presenter_->framesInFlight());
    // the format is settled here even though the image may never be built, because every
    // pipeline that could draw into a depth pass is built against it
    depthFormat_ = vulkan::frame::DepthBuffer::chooseFormat(device_->physical());
    // dynamic rendering has no render pass to take the target format from, so a pipeline
    // is built against the chain's. Recreating the chain keeps that format
    quads_ = boost::make_shared<vulkan::renderer::Quad>(logger, device_, pipelineCache_, resources_, presenter_,
        frameUniforms_, swapchain_->format(), depthFormat_);
}

/**
 **/
Context3D::~Context3D() {
    // the device may still be drawing with everything about to be destroyed
    if (presenter_) {
        presenter_->waitIdle();
    }
}

/**
 **/
boost::shared_ptr<vulkan::device::Device> Context3D::device() const {
    return device_;
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
boost::shared_ptr<vulkan::pipeline::Cache> Context3D::pipelineCache() const {
    return pipelineCache_;
}

/**
 **/
boost::shared_ptr<vulkan::pipeline::Resources> Context3D::resources() const {
    return resources_;
}

/**
 **/
boost::shared_ptr<vulkan::renderer::Quad> Context3D::quads() const {
    return quads_;
}

/**
 **/
boost::shared_ptr<vulkan::frame::FrameUniforms> Context3D::frameUniforms() const {
    return frameUniforms_;
}

/**
 **/
boost::shared_ptr<vulkan::memory::Uploader> Context3D::uploader() const {
    return uploader_;
}

/**
 **/
VkFormat Context3D::depthFormat() const noexcept {
    return depthFormat_;
}

/**
 **/
boost::shared_ptr<vulkan::renderer::Line> Context3D::lines() {
    if (!lines_) {
        lines_ = boost::make_shared<vulkan::renderer::Line>(logger_, device_, pipelineCache_, resources_, presenter_,
            frameUniforms_, swapchain_->format(), depthFormat_);
    }
    return lines_;
}

/**
 **/
bool Context3D::hasLines() const noexcept {
    return static_cast<bool>(lines_);
}

/**
 **/
boost::shared_ptr<vulkan::renderer::World> Context3D::worldQuads() {
    if (!worldQuads_) {
        worldQuads_ = boost::make_shared<vulkan::renderer::World>(logger_, device_, pipelineCache_, resources_,
            presenter_, frameUniforms_, quads_, swapchain_->format(), depthFormat_);
    }
    return worldQuads_;
}

/**
 **/
bool Context3D::hasWorldQuads() const noexcept {
    return static_cast<bool>(worldQuads_);
}

/**
 **/
boost::shared_ptr<vulkan::frame::DepthBuffer> Context3D::depth() {
    if (!depth_) {
        // the chain's extent rather than the window's - the surface is allowed to dictate
        // one that is not what was asked for, and the two attachments have to agree
        depth_ = boost::make_shared<vulkan::frame::DepthBuffer>(device_, swapchain_->extent().width, swapchain_->extent().height);
    }
    return depth_;
}

/**
 **/
bool Context3D::hasDepth() const noexcept {
    return static_cast<bool>(depth_);
}

/**
 **/
void Context3D::resize() {
    swapchain_->recreate(static_cast<uint32_t>(window_->width()), static_cast<uint32_t>(window_->height()));
    // a new chain can hold a different number of images, and the presenter keeps a
    // semaphore per image
    presenter_->reset();
    if (depth_) {
        // the depth buffer is only correct while it is the size of what it is attached to
        depth_->recreate(swapchain_->extent().width, swapchain_->extent().height);
    }
}
};  // namespace v3d::render::realtime

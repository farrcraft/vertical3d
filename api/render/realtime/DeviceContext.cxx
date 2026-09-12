/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "DeviceContext.h"

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {
/**
 **/
DeviceContext::DeviceContext(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<vulkan::device::Device>& device,
    VkFormat colour, const VkExtent2D& extent, uint32_t framesInFlight) :
logger_(logger),
device_(device),
colourFormat_(colour),
extent_(extent),
depthFormat_(VK_FORMAT_UNDEFINED) {
    ring_ = boost::make_shared<vulkan::frame::Ring>(device_, framesInFlight);
    pipelineCache_ = boost::make_shared<vulkan::pipeline::Cache>(device_);
    resources_ = boost::make_shared<vulkan::pipeline::Resources>(device_);
    uploader_ = boost::make_shared<vulkan::memory::Uploader>(device_);
    frameUniforms_ = boost::make_shared<vulkan::frame::FrameUniforms>(device_, ring_->framesInFlight());
    // settled here even though the image may never be built, because every pipeline that
    // could draw into a depth pass is built against it
    depthFormat_ = vulkan::frame::DepthBuffer::chooseFormat(device_->physical());
}

/**
 **/
DeviceContext::~DeviceContext() {
    // the device may still be drawing with everything about to be destroyed
    if (ring_) {
        ring_->waitIdle();
    }
}

/**
 **/
boost::shared_ptr<vulkan::device::Device> DeviceContext::device() const {
    return device_;
}

/**
 **/
boost::shared_ptr<vulkan::frame::Ring> DeviceContext::ring() const {
    return ring_;
}

/**
 **/
boost::shared_ptr<vulkan::pipeline::Cache> DeviceContext::pipelineCache() const {
    return pipelineCache_;
}

/**
 **/
boost::shared_ptr<vulkan::pipeline::Resources> DeviceContext::resources() const {
    return resources_;
}

/**
 **/
boost::shared_ptr<vulkan::frame::FrameUniforms> DeviceContext::frameUniforms() const {
    return frameUniforms_;
}

/**
 **/
boost::shared_ptr<vulkan::memory::Uploader> DeviceContext::uploader() const {
    return uploader_;
}

/**
 **/
VkFormat DeviceContext::colourFormat() const noexcept {
    return colourFormat_;
}

/**
 **/
const VkExtent2D& DeviceContext::extent() const noexcept {
    return extent_;
}

/**
 **/
VkFormat DeviceContext::depthFormat() const noexcept {
    return depthFormat_;
}

/**
 **/
void DeviceContext::describe(VkFormat colour, const VkExtent2D& extent) noexcept {
    colourFormat_ = colour;
    extent_ = extent;
}

/**
 **/
boost::shared_ptr<vulkan::frame::DepthBuffer> DeviceContext::depth() {
    if (!depth_) {
        depth_ = boost::make_shared<vulkan::frame::DepthBuffer>(device_, extent_.width, extent_.height);
    }
    return depth_;
}

/**
 **/
bool DeviceContext::hasDepth() const noexcept {
    return static_cast<bool>(depth_);
}

/**
 **/
boost::shared_ptr<vulkan::renderer::Quad> DeviceContext::quads() {
    if (!quads_) {
        quads_ = boost::make_shared<vulkan::renderer::Quad>(logger_, device_, pipelineCache_, resources_, ring_,
            frameUniforms_, colourFormat_, depthFormat_);
    }
    return quads_;
}

/**
 **/
bool DeviceContext::hasQuads() const noexcept {
    return static_cast<bool>(quads_);
}

/**
 **/
boost::shared_ptr<vulkan::renderer::Line> DeviceContext::lines() {
    if (!lines_) {
        lines_ = boost::make_shared<vulkan::renderer::Line>(logger_, device_, pipelineCache_, resources_, ring_,
            frameUniforms_, colourFormat_, depthFormat_);
    }
    return lines_;
}

/**
 **/
bool DeviceContext::hasLines() const noexcept {
    return static_cast<bool>(lines_);
}

/**
 **/
boost::shared_ptr<vulkan::renderer::World> DeviceContext::worldQuads() {
    if (!worldQuads_) {
        worldQuads_ = boost::make_shared<vulkan::renderer::World>(logger_, device_, pipelineCache_, resources_,
            ring_, frameUniforms_, quads(), colourFormat_, depthFormat_);
    }
    return worldQuads_;
}

/**
 **/
bool DeviceContext::hasWorldQuads() const noexcept {
    return static_cast<bool>(worldQuads_);
}

};  // namespace v3d::render::realtime

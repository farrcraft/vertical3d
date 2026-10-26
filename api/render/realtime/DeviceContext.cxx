/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "DeviceContext.h"

#include <api/log/Logger.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/DepthBuffer.h>
#include <api/render/realtime/vulkan/frame/FrameUniforms.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/memory/Uploader.h>
#include <api/render/realtime/vulkan/pipeline/Cache.h>
#include <api/render/realtime/Resources.h>
#include <api/render/realtime/renderer/Line.h>
#include <api/render/realtime/renderer/Quad.h>
#include <api/render/realtime/renderer/World.h>

#include "Textures.h"

#include <cstdint>

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
    resources_ = boost::make_shared<Resources>(device_, ring_);
    uploader_ = boost::make_shared<vulkan::memory::Uploader>(device_, ring_);
    textures_ = boost::make_shared<Textures>(device_, resources_, ring_, uploader_);
    frameUniforms_ = boost::make_shared<vulkan::frame::FrameUniforms>(device_, ring_);
    // settled here even though the image may never be built, because every pipeline that
    // could draw into a depth pass is built against it
    depthFormat_ = vulkan::frame::DepthBuffer::chooseFormat(device_->physical());
}

/**
 **/
DeviceContext::~DeviceContext() {
    // the device may still be drawing with everything about to be destroyed
    if (ring_) {
        ring_->waitIdleNoThrow();
    }
}

/**
 **/
boost::shared_ptr<vulkan::device::Device> DeviceContext::device() const {
    return device_;
}

/**
 **/
boost::shared_ptr<v3d::log::Logger> DeviceContext::logger() const {
    return logger_;
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
boost::shared_ptr<Resources> DeviceContext::resources() const {
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
void DeviceContext::describe(VkFormat colour, const VkExtent2D& extent) {
    const bool resized = extent.width != extent_.width || extent.height != extent_.height;
    colourFormat_ = colour;
    extent_ = extent;
    if (depth_ && resized) {
        // the depth buffer is only correct while it is the size of what it is attached to. A
        // frame in flight may still be testing against the old one
        const boost::shared_ptr<vulkan::frame::DepthBuffer> outgrown = depth_;
        depth_ = boost::make_shared<vulkan::frame::DepthBuffer>(device_, extent_.width, extent_.height);
        ring_->retire([outgrown]() {});
    }
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
boost::shared_ptr<Textures> DeviceContext::textures() const {
    return textures_;
}

/**
 **/
boost::shared_ptr<renderer::Quad> DeviceContext::quads() {
    if (!quads_) {
        quads_ = boost::make_shared<renderer::Quad>(*this, renderer::Quad::Spec{.colour = colourFormat_, .depth = depthFormat_});
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
boost::shared_ptr<renderer::Line> DeviceContext::lines() {
    if (!lines_) {
        lines_ = boost::make_shared<renderer::Line>(*this, renderer::Line::Spec{.colour = colourFormat_, .depth = depthFormat_});
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
boost::shared_ptr<renderer::World> DeviceContext::worldQuads() {
    if (!worldQuads_) {
        worldQuads_ = boost::make_shared<renderer::World>(*this, renderer::World::Spec{.colour = colourFormat_, .depth = depthFormat_});
    }
    return worldQuads_;
}

/**
 **/
bool DeviceContext::hasWorldQuads() const noexcept {
    return static_cast<bool>(worldQuads_);
}

};  // namespace v3d::render::realtime

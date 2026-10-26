/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Engine3D.h"

#include <api/asset/Manager.h>
#include <api/log/Logger.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/DepthBuffer.h>
#include <api/render/realtime/vulkan/frame/FrameUniforms.h>
#include <api/render/realtime/vulkan/frame/Presenter.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/Timings.h>
#include <api/render/realtime/vulkan/memory/Allocator.h>
#include <api/render/realtime/vulkan/renderer/Line.h>
#include <api/render/realtime/vulkan/renderer/Quad.h>
#include <api/render/realtime/vulkan/renderer/World.h>
#include <api/window/Window.h>

#include "Context3D.h"
#include "DeviceContext.h"
#include "Engine.h"
#include "Frame.h"
#include "Present.h"
#include "Textures.h"

#include <string>
#include <string_view>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {

namespace {

constexpr glm::vec4 DEFAULT_CLEAR_COLOUR{0.06F, 0.07F, 0.10F, 1.0F};

};  // namespace

const char* const Engine3D::colourPass = "colour";

/**
 **/
Engine3D::Engine3D(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager,
    VkFormat preferred, vulkan::memory::Allocator::Kind allocations) :
    Engine(logger, assetManager),
    clearColour_(DEFAULT_CLEAR_COLOUR),
    preferred_(preferred),
    allocations_(allocations),
    present_(Present::Uncapped) {
}

/**
 **/
bool Engine3D::initialize(const boost::shared_ptr<v3d::window::Window>& window) {
    Engine::initialize(window);

    // the context can only be built once there is a created window to take a device from
    context_ = boost::make_shared<Context3D>(logger(), window, preferred_, allocations_, present_);
    pendingPresent_.reset();

    frame_ = boost::make_shared<Frame>();
    frame_->pass(colourPass)->clearColour(clearColour_);

    return true;
}

/**
 **/
bool Engine3D::shutdown() {
    if (context_) {
        // every submitted frame has to finish before the swapchain, the device and the
        // window are destroyed
        context_->ring()->waitIdleNoThrow();
    }
    // the context must be destroyed before the window. The context owns the device, which
    // holds the surface made for the window, and a surface has to go before its window does.
    // assigned null rather than calling frame_.reset(), which reads like Frame::reset()
    frame_ = nullptr;
    // the capture's readback buffer belongs to the device the context owns
    capture_.reset();
    capturePath_.clear();
    context_.reset();
    return Engine::shutdown();
}

/**
 **/
boost::shared_ptr<DeviceContext> Engine3D::context() {
    return context_;
}

/**
 **/
boost::shared_ptr<Frame> Engine3D::frame() const {
    return frame_;
}

/**
 **/
const std::vector<vulkan::frame::Timings::Timing>& Engine3D::timings() const {
    static const std::vector<vulkan::frame::Timings::Timing> none;
    return context_ ? context_->ring()->timings().last() : none;
}

/**
 **/
boost::shared_ptr<vulkan::renderer::Quad> Engine3D::quads() const {
    return context_ ? context_->quads() : boost::shared_ptr<vulkan::renderer::Quad>();
}

/**
 **/
boost::shared_ptr<Textures> Engine3D::textures() const {
    return context_ ? context_->textures() : boost::shared_ptr<Textures>();
}

/**
 **/
boost::shared_ptr<vulkan::renderer::Line> Engine3D::lines() {
    return context_ ? context_->lines() : boost::shared_ptr<vulkan::renderer::Line>();
}

/**
 **/
boost::shared_ptr<vulkan::renderer::World> Engine3D::worldQuads() {
    return context_ ? context_->worldQuads() : boost::shared_ptr<vulkan::renderer::World>();
}

/**
 **/
void Engine3D::clearColour(const glm::vec4& colour) {
    clearColour_ = colour;
    if (frame_) {
        frame_->pass(colourPass)->clearColour(colour);
    }
}

/**
 **/
bool Engine3D::capture(std::string_view path) {
    if (!context_ || !context_->swapchain()->copyable()) {
        return false;
    }
    if (!capture_) {
        capture_ = boost::make_shared<vulkan::frame::Capture>(context_->device(), logger());
    }
    capturePath_ = path;
    return true;
}

/**
 **/
bool Engine3D::beginFrame(glm::ivec2* size) {
    const boost::shared_ptr<v3d::window::Window> target = window();
    const int width = target ? target->width() : 0;
    const int height = target ? target->height() : 0;
    if (width <= 0 || height <= 0) {
        renderFrame();
        return false;
    }
    if (size != nullptr) {
        *size = glm::ivec2(width, height);
    }
    return true;
}

/**
 **/
void Engine3D::present(Present present) {
    present_ = present;
    if (context_) {
        pendingPresent_ = present;
    }
}

/**
 **/
Present Engine3D::presenting() const noexcept {
    return context_ ? context_->swapchain()->presenting() : present_;
}

/**
 **/
void Engine3D::renderFrame() {
    if (!context_ || !frame_) {
        return;
    }

    if (pendingPresent_) {
        // a new present mode needs a new swapchain, which is built here rather than when it
        // was asked for, so that nothing is rebuilt while a tick is building the frame
        context_->present(*pendingPresent_);
        pendingPresent_.reset();
        context_->ring()->skip();
        endFrame();
        return;
    }

    const boost::shared_ptr<vulkan::frame::Presenter> presenter = context_->presenter();

    vulkan::frame::Presenter::Acquisition acquisition;
    const vulkan::frame::Presenter::Status status = presenter->acquire(&acquisition);

    if (status == vulkan::frame::Presenter::Status::OutOfDate) {
        // the window changed size since the last present. Recreate the swapchain, and draw
        // into it next frame
        context_->resize();
        context_->ring()->skip();
        endFrame();
        return;
    }

    if (status == vulkan::frame::Presenter::Status::Skip) {
        // there is no swapchain, because the window had no area when the swapchain was last
        // created. Once the window has an area again, as when it is restored from minimized,
        // recreate the swapchain
        if (window() && window()->width() > 0 && window()->height() > 0) {
            context_->resize();
        }
        context_->ring()->skip();
        endFrame();
        return;
    }

    const boost::shared_ptr<vulkan::frame::Swapchain> swapchain = context_->swapchain();

    vulkan::frame::Recorder::Target target;
    target.image = swapchain->images()[acquisition.image];
    target.view = swapchain->views()[acquisition.image];
    target.extent = swapchain->extent();
    target.format = swapchain->format();

    // the depth buffer is allocated the first frame a pass asks for one, so an app that
    // never depth tests never pays for a full screen image it does not read
    if (frame_->swapchainDepth()) {
        const boost::shared_ptr<vulkan::frame::DepthBuffer> buffer = context_->depth();
        if (buffer->valid()) {
            target.depthImage = buffer->image();
            target.depthView = buffer->view();
            target.depthFormat = buffer->format();
        }
    }

    const boost::shared_ptr<vulkan::frame::FrameUniforms> uniforms = context_->frameUniforms();
    // the slots of the frame about to be recorded are free - acquire() waited on its fence
    uniforms->begin(context_->ring()->frame());

    vulkan::frame::Recorder::record(acquisition.commands, *frame_, target, *context_->resources(), uniforms.get(),
        &context_->ring()->timings());

    // a swapchain recreated since the request may have lost TRANSFER_SRC, and Capture throws
    // for that
    const bool capturing = !capturePath_.empty() && swapchain->copyable();
    if (!capturePath_.empty() && !capturing) {
        logger()->get()->error("Cannot capture a frame to {}: the swapchain images cannot be copied out of", capturePath_);
        capturePath_.clear();
    }
    if (capturing) {
        // after Recorder::record, which leaves the image in PRESENT_SRC as the copy expects
        capture_->record(acquisition.commands, *swapchain, acquisition.image);
    }

    // present() submits even when it returns OutOfDate, so a capture recorded into the frame
    // is written either way
    const bool outOfDate = presenter->present(acquisition) == vulkan::frame::Presenter::Status::OutOfDate;

    if (capturing) {
        context_->ring()->waitIdle();
        if (!capture_->write(capturePath_)) {
            logger()->get()->error("Cannot write a captured frame to {}", capturePath_);
        }
        capturePath_.clear();
    }

    if (outOfDate) {
        context_->resize();
    }

    endFrame();
}

/**
 **/
void Engine3D::endFrame() {
    frame_->reset();
}

};  // namespace v3d::render::realtime

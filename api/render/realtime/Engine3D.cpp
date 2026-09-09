/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Engine3D.h"

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {

const char* const Engine3D::colourPass = "colour";

/**
 **/
Engine3D::Engine3D(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry) :
    Engine(logger, assetManager, registry),
    clearColour_(0.06f, 0.07f, 0.10f, 1.0f) {
}

/**
 **/
bool Engine3D::initialize(const boost::shared_ptr<Window>& window) {
    Engine::initialize(window);

    // the context can only be built once there is a created window to take a device from
    context_ = boost::make_shared<Context3D>(logger(), window);

    frame_ = boost::make_shared<Frame>(context_);
    frame_->pass(colourPass)->clearColour(clearColour_);

    return true;
}

/**
 **/
bool Engine3D::shutdown() {
    if (context_) {
        // the swapchain, the device and the window all outlive the frames that were
        // submitted against them, but only just
        context_->presenter()->waitIdle();
    }
    // the context has to go before the window does. It owns the device, which holds
    // the window's surface alive, and the window's teardown unloads the vulkan library -
    // a surface destroyed after that is not destroyed at all, and the instance reports
    // it as leaked
    // released, not Frame::reset() - the assignment is what tells the two apart at a glance
    frame_ = nullptr;
    context_.reset();
    return Engine::shutdown();
}

/**
 **/
boost::shared_ptr<Context> Engine3D::context() {
    return context_;
}

/**
 **/
boost::shared_ptr<Frame> Engine3D::frame() const {
    return frame_;
}

/**
 **/
boost::shared_ptr<vulkan::renderer::Quad> Engine3D::quads() const {
    return context_ ? context_->quads() : boost::shared_ptr<vulkan::renderer::Quad>();
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
bool Engine3D::beginFrame(glm::ivec2* size) {
    const boost::shared_ptr<Window> target = window();
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
void Engine3D::renderFrame() {
    if (!context_ || !frame_) {
        return;
    }

    boost::shared_ptr<vulkan::frame::Presenter> presenter = context_->presenter();

    vulkan::frame::Presenter::Acquisition acquisition;
    const vulkan::frame::Presenter::Status status = presenter->acquire(&acquisition);

    if (status == vulkan::frame::Presenter::Status::OutOfDate) {
        // the window changed size between the last present and this acquire - rebuild
        // the chain and let the next frame draw into it
        context_->resize();
        endFrame();
        return;
    }

    if (status == vulkan::frame::Presenter::Status::Skip) {
        // there is no chain, which means the window had no area when it was last built.
        // if it has one again - a minimized window that came back - build one for it
        if (window() && window()->width() > 0 && window()->height() > 0) {
            context_->resize();
        }
        endFrame();
        return;
    }

    const boost::shared_ptr<vulkan::frame::Swapchain> swapchain = context_->swapchain();

    vulkan::frame::Recorder::Target target;
    target.image = swapchain->images()[acquisition.image];
    target.view = swapchain->views()[acquisition.image];
    target.extent = swapchain->extent();

    // the depth buffer is allocated the first frame a pass asks for one, so an app that
    // never depth tests never pays for a full screen image it does not read
    bool depth = false;
    for (const boost::shared_ptr<Pass>& pass : frame_->passes()) {
        if (pass->depth()) {
            depth = true;
            break;
        }
    }
    if (depth) {
        const boost::shared_ptr<vulkan::frame::DepthBuffer> buffer = context_->depth();
        if (buffer->valid()) {
            target.depthImage = buffer->image();
            target.depthView = buffer->view();
        }
    }

    const boost::shared_ptr<vulkan::frame::FrameUniforms> uniforms = context_->frameUniforms();
    // the slots of the frame about to be recorded are free - acquire() waited on its fence
    uniforms->begin(presenter->frame());

    recorder_.record(acquisition.commands, *frame_, target, *context_->resources(), uniforms.get());

    if (presenter->present(acquisition) == vulkan::frame::Presenter::Status::OutOfDate) {
        context_->resize();
    }

    endFrame();
}

/**
 **/
void Engine3D::endFrame() {
    frame_->reset();
    // the geometry buffers this frame's submissions took go back to the front of
    // their rings, for the next frame to claim from
    if (context_) {
        context_->quads()->endFrame();
        if (context_->hasLines()) {
            context_->lines()->endFrame();
        }
        if (context_->hasWorldQuads()) {
            context_->worldQuads()->endFrame();
        }
    }
}

};  // namespace v3d::render::realtime

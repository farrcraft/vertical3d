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
    bool Engine3D::initialize(const boost::shared_ptr <Window3D>& window) {
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
    boost::shared_ptr<vulkan::QuadRenderer> Engine3D::quads() const {
        return context_ ? context_->quads() : boost::shared_ptr<vulkan::QuadRenderer>();
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
    void Engine3D::renderFrame() {
        if (!context_ || !frame_) {
            return;
        }

        boost::shared_ptr<vulkan::Presenter> presenter = context_->presenter();

        vulkan::Presenter::Acquisition acquisition;
        const vulkan::Presenter::Status status = presenter->acquire(&acquisition);

        if (status == vulkan::Presenter::Status::OutOfDate) {
            // the window changed size between the last present and this acquire - rebuild
            // the chain and let the next frame draw into it
            context_->resize();
            frame_->reset();
            return;
        }

        if (status == vulkan::Presenter::Status::Skip) {
            // there is no chain, which means the window had no area when it was last built.
            // if it has one again - a minimized window that came back - build one for it
            if (window() && window()->width() > 0 && window()->height() > 0) {
                context_->resize();
            }
            frame_->reset();
            return;
        }

        const boost::shared_ptr<vulkan::Swapchain> swapchain = context_->swapchain();

        vulkan::Recorder::Target target;
        target.image = swapchain->images()[acquisition.image];
        target.view = swapchain->views()[acquisition.image];
        target.extent = swapchain->extent();

        recorder_.record(acquisition.commands, *frame_, target, *context_->resources());

        if (presenter->present(acquisition) == vulkan::Presenter::Status::OutOfDate) {
            context_->resize();
        }

        frame_->reset();
    }

};  // namespace v3d::render::realtime

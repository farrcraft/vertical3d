/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Context.h"
#include "Context3D.h"
#include "Engine.h"
#include "Frame.h"
#include "Window.h"
#include "vulkan/Recorder.h"

#include <glm/vec4.hpp>

namespace v3d::render::realtime {
    /* A 3D render engine.
     **/
    class Engine3D : public Engine {
     public:
        /**
         **/
        Engine3D(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry);

        /**
         **/
        bool initialize(const boost::shared_ptr<Window>& window);

        /**
         * Wait for everything in flight before the device and the window go away.
         **/
        bool shutdown() override;

        /**
         * Record the frame that has been built up, present it, and empty it ready for the
         * next one. Rebuilds the swapchain when the window has changed size underneath it,
         * and draws nothing at all while the window has no area.
         **/
        void renderFrame() override;

        /**
         * The frame being built for the next present. Passes and draw items are added to
         * this during a tick, and recorded in one step by renderFrame().
         **/
        boost::shared_ptr<Frame> frame() const;

        /**
         * The colour the frame's first pass clears to.
         **/
        void clearColour(const glm::vec4& colour);

        /**
         * The batched quad primitive every 2D thing draws through - ADR-0005. An app fills a
         * Canvas during its tick and hands both to this.
         **/
        boost::shared_ptr<vulkan::QuadRenderer> quads() const;

        /**
         * The name of the pass every frame has, for an app adding items to it directly.
         **/
        static const char* const colourPass;

        boost::shared_ptr<Context> context();

     private:
        boost::shared_ptr<Context3D> context_;
        boost::shared_ptr<Frame> frame_;
        vulkan::Recorder recorder_;
        glm::vec4 clearColour_;
    };
};  // namespace v3d::render::realtime

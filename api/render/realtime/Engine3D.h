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

#include <glm/vec2.hpp>
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
     * Whether there is a frame worth building, and how big it is.
     *
     * A minimized window has no area, and a canvas with none has no projection to build
     * geometry against. When this returns false the frame has already been presented -
     * empty - so the caller draws nothing and returns.
     *
     * No resize event reaches a renderer, so this is also where an app learns that the
     * window it is drawing into has changed size.
     *
     * @param size where the window's size in pixels is written, when there is a frame
     * @return false when the frame was skipped
     **/
    bool beginFrame(glm::ivec2* size);

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
     * The line primitive of ADR-0011. An app fills a LineCanvas during its tick and
     * hands both to this.
     *
     * Built on the first call rather than at startup, so an app that draws no lines pays
     * nothing for it.
     **/
    boost::shared_ptr<vulkan::LineRenderer> lines();

    /**
     * The name of the pass every frame has, for an app adding items to it directly.
     **/
    static const char* const colourPass;

    boost::shared_ptr<Context> context();

 private:
    /**
     * Drop what the frame collected and give back the geometry buffers its submissions
     * took, whether or not the frame was recorded.
     **/
    void endFrame();

    boost::shared_ptr<Context3D> context_;
    boost::shared_ptr<Frame> frame_;
    vulkan::Recorder recorder_;
    glm::vec4 clearColour_;
};
};  // namespace v3d::render::realtime

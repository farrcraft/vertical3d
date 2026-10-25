/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/Present.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/memory/Allocator.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Context3D.h"
#include "Engine.h"
#include "Frame.h"
#include "Window.h"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime {
/* A 3D render engine.
 **/
class Engine3D : public Engine {
 public:
    /**
     * @param logger
     * @param assetManager
     * @param preferred the colour format to present through. An app whose shaders write
     *        linear light names one here. Leaving it undefined selects the display space
     *        (UNORM) default.
     * @param allocations how the device finds memory for its buffers and images. Suballocated
     *        unless an app asks for one allocation per resource.
     **/
    Engine3D(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager,
        VkFormat preferred = VK_FORMAT_UNDEFINED,
        vulkan::memory::Allocator::Kind allocations = vulkan::memory::Allocator::Kind::Suballocated);

    /**
     **/
    bool initialize(const boost::shared_ptr<Window>& window) override;

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
     * Write the next frame that is presented to a png.
     *
     * The frame is copied after it is recorded and before it is presented, so the file holds
     * exactly what reached the window. A frame that is skipped, because the window has no area
     * or the swapchain is being recreated, keeps the request for the next one that presents.
     * Once a frame has tried to write the file the request is cleared, so a path that cannot
     * be written fails once and is reported in the log.
     *
     * The frame waits for the device to go idle before it reads the copy back, so a frame that
     * is captured does not overlap the next one.
     *
     * @param path where the png is written
     * @return false, and nothing is requested, before initialize() or when the swapchain's
     *         images cannot be copied out of because the surface does not allow it
     **/
    bool capture(std::string_view path);

    /**
     * Ask for how the window's images are presented, which is a settings screen's vsync.
     *
     * Before initialize(), the swapchain is built with it. After, the next renderFrame()
     * rebuilds the swapchain to take it, as a resize does, and draws nothing that frame. The
     * default is Present::Uncapped. A driver need not offer every mode, so presenting() says
     * which was granted.
     **/
    void present(Present present);

    /**
     * @return the present mode the swapchain was granted, or the one requested before
     *         initialize(). A request not yet taken by renderFrame() is not reported until it is
     **/
    Present presenting() const noexcept;

    /**
     * Whether there is a frame worth building, and how big it is.
     *
     * A minimized window has no area, and a canvas with none has no projection to build
     * geometry against. When this returns false, it has already called renderFrame() for the
     * empty frame, so the caller draws nothing and returns.
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
     * How long the device spent on each pass of a frame, by the pass's name. The numbers are
     * framesInFlight frames old, and the list is empty on a device that writes no timestamps.
     **/
    const std::vector<vulkan::frame::Timings::Timing>& timings() const;

    /**
     * The colour the frame's first pass clears to.
     **/
    void clearColour(const glm::vec4& colour);

    /**
     * The batched quad primitive that all 2D drawing goes through. An app fills a Canvas during
     * its tick and hands both to this.
     **/
    boost::shared_ptr<vulkan::renderer::Quad> quads() const;

    /**
     * @return where a texture is uploaded and made a material, or null before initialize()
     **/
    boost::shared_ptr<Textures> textures() const;

    /**
     * The world space line primitive. An app fills a canvas::Line during its tick and hands
     * both to this.
     *
     * Built on the first call rather than at startup, so an app that draws no lines pays
     * nothing for it.
     **/
    boost::shared_ptr<vulkan::renderer::Line> lines();

    /**
     * The world space quad primitive. An app fills a canvas::World during its tick and hands
     * both to this.
     *
     * Built on the first call rather than at startup, so an app that draws none pays
     * nothing for it.
     **/
    boost::shared_ptr<vulkan::renderer::World> worldQuads();

    /**
     * The name of the pass every frame has, for an app adding items to it directly.
     **/
    static const char* const colourPass;

    boost::shared_ptr<DeviceContext> context();

 private:
    /**
     * Drop what the frame collected, whether or not the frame was recorded.
     **/
    void endFrame();

    boost::shared_ptr<Context3D> context_;
    boost::shared_ptr<Frame> frame_;
    glm::vec4 clearColour_;
    /**< the format initialize() requests for the swapchain. The surface decides which it gets **/
    VkFormat preferred_;
    /**< what initialize() builds the device's allocator as **/
    vulkan::memory::Allocator::Kind allocations_;
    /**< the present mode initialize() asks for **/
    Present present_;
    /**< a present mode asked for since initialize(), which the next renderFrame() takes **/
    std::optional<Present> pendingPresent_;
    /**< made by the first capture() and kept, so its readback buffer is reused **/
    boost::shared_ptr<vulkan::frame::Capture> capture_;
    /**< where the next presented frame is written, or empty when none is asked for **/
    std::string capturePath_;
};
};  // namespace v3d::render::realtime

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Context.h"
#include "Window.h"
#include "vulkan/DepthBuffer.h"
#include "vulkan/Device.h"
#include "vulkan/FrameUniforms.h"
#include "vulkan/LineRenderer.h"
#include "vulkan/PipelineCache.h"
#include "vulkan/Presenter.h"
#include "vulkan/QuadRenderer.h"
#include "vulkan/WorldRenderer.h"
#include "vulkan/Resources.h"
#include "vulkan/Swapchain.h"
#include "vulkan/Uploader.h"

#include "../../log/Logger.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
/**
 * The 3D render context - owns the vulkan device the window is drawn with, the chain of
 * images presented to it, and the per frame machinery that gets one onto the screen.
 **/
class Context3D : public Context {
 public:
    /**
     * @param logger
     * @param window the window the context renders to
     **/
    Context3D(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Window>& window);

    /**
     **/
    ~Context3D();

    /**
     * @return the device backing the context
     **/
    boost::shared_ptr<vulkan::Device> device() const;

    /**
     * @return the chain of images being presented to the window
     **/
    boost::shared_ptr<vulkan::Swapchain> swapchain() const;

    /**
     * @return the acquire, submit and present loop the frames go through
     **/
    boost::shared_ptr<vulkan::Presenter> presenter() const;

    /**
     * @return the cache every pipeline built on this device is compiled against
     **/
    boost::shared_ptr<vulkan::PipelineCache> pipelineCache() const;

    /**
     * @return the pipelines, materials and textures a draw item can name by handle
     **/
    boost::shared_ptr<vulkan::Resources> resources() const;

    /**
     * @return the batched quad primitive of ADR-0005, which every 2D thing draws through
     **/
    boost::shared_ptr<vulkan::QuadRenderer> quads() const;

    /**
     * The line primitive of ADR-0011, built on the first call and kept from then on.
     *
     * Lazy for the same reason the depth buffer is: an app that draws no lines would
     * otherwise pay two pipeline compiles and a vertex buffer per frame in flight for
     * nothing.
     *
     * @return the renderer, which every line in the engine draws through
     * @throw std::runtime_error if its pipelines cannot be created
     **/
    boost::shared_ptr<vulkan::LineRenderer> lines();

    /**
     * @return whether a line renderer has been built, without building one
     **/
    bool hasLines() const noexcept;

    /**
     * The world space quad primitive of ADR-0042, built on the first call and kept from
     * then on. Lazy for the same reason the line renderer is.
     *
     * @return the renderer, which every world space quad draws through
     * @throw std::runtime_error if its pipelines cannot be created
     **/
    boost::shared_ptr<vulkan::WorldRenderer> worldQuads();

    /**
     * @return whether a world quad renderer has been built, without building one
     **/
    bool hasWorldQuads() const noexcept;

    /**
     * @return set 0, where each pass's camera is written and bound from - ADR-0008
     **/
    boost::shared_ptr<vulkan::FrameUniforms> frameUniforms() const;

    /**
     * @return the one-shot queue everything reaching device local memory is copied by
     **/
    boost::shared_ptr<vulkan::Uploader> uploader() const;

    /**
     * The format a pipeline that depth tests has to be built against.
     *
     * Known from the device rather than from the image, so a pipeline can be built
     * before anything has asked for a depth buffer.
     **/
    VkFormat depthFormat() const noexcept;

    /**
     * The depth buffer, allocated on the first call and sized with the swapchain from
     * then on.
     *
     * Lazy because a 2D app never asks: pong and tetris draw painter ordered quads and
     * would otherwise pay a full screen depth image for nothing.
     *
     * @return the buffer, which may be invalid if the window has no area
     **/
    boost::shared_ptr<vulkan::DepthBuffer> depth();

    /**
     * @return whether a depth buffer has been allocated, without allocating one
     **/
    bool hasDepth() const noexcept;

    /**
     * Rebuild the swapchain against the window's current size, and everything that is
     * sized by it. Call this when presenting reports the chain has gone out of date.
     **/
    void resize();

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<Window> window_;
    boost::shared_ptr<vulkan::Device> device_;
    boost::shared_ptr<vulkan::Swapchain> swapchain_;
    boost::shared_ptr<vulkan::PipelineCache> pipelineCache_;
    boost::shared_ptr<vulkan::Resources> resources_;
    boost::shared_ptr<vulkan::Uploader> uploader_;
    boost::shared_ptr<vulkan::FrameUniforms> frameUniforms_;
    boost::shared_ptr<vulkan::DepthBuffer> depth_;
    VkFormat depthFormat_;
    boost::shared_ptr<vulkan::QuadRenderer> quads_;
    boost::shared_ptr<vulkan::LineRenderer> lines_;
    boost::shared_ptr<vulkan::WorldRenderer> worldQuads_;
    // last, so that it is torn down first - nothing else may go away while a frame it
    // submitted is still in flight
    boost::shared_ptr<vulkan::Presenter> presenter_;
};
};  // namespace v3d::render::realtime

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/DepthBuffer.h>
#include <api/render/realtime/vulkan/frame/FrameUniforms.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/memory/Uploader.h>
#include <api/render/realtime/vulkan/pipeline/Cache.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>
#include <api/render/realtime/vulkan/renderer/Line.h>
#include <api/render/realtime/vulkan/renderer/Quad.h>
#include <api/render/realtime/vulkan/renderer/World.h>

#include "Context.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
/**
 * Everything a context owns that needs a device and nothing more: the frames in flight, the
 * pipeline cache and resources, the uploader, set 0, and the three renderers.
 *
 * This is usable on its own. A context that draws into a window is Context3D, which adds the
 * chain and the presenter; a context that draws into an offscreen target is this, told what
 * format and size that target is. Nothing here knows which it is - under dynamic rendering a
 * pipeline is built against the format of whatever it draws into, and that is the whole of
 * what a renderer needs to be told about its destination.
 *
 * Every renderer is built on the first call for it and kept. An app that draws no lines would
 * otherwise pay two pipeline compiles and a vertex buffer per frame in flight for nothing, and
 * a context built before its destination is described has nothing to build them against yet.
 **/
class DeviceContext : public Context {
 public:
    /**
     * @param logger
     * @param device the device everything here is built on
     * @param colour the colour format of what this draws into, and what every pipeline built
     *        here is built against. A destination known when the context is built says so
     *        here; Context3D cannot, because its chain does not exist yet
     * @param extent the size of that destination
     * @param framesInFlight how many frames may be recorded ahead of the device
     **/
    DeviceContext(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<vulkan::device::Device>& device,
        VkFormat colour = VK_FORMAT_UNDEFINED, const VkExtent2D& extent = VkExtent2D{0, 0}, uint32_t framesInFlight = 2);

    /**
     **/
    ~DeviceContext() override;

    /**
     * @return the device backing the context
     **/
    boost::shared_ptr<vulkan::device::Device> device() const;

    /**
     * @return the frames recorded ahead of the device - ADR-0051. Anything keeping a resource
     *         per frame in flight indexes it by this
     **/
    boost::shared_ptr<vulkan::frame::Ring> ring() const;

    /**
     * @return the cache every pipeline built on this device is compiled against
     **/
    boost::shared_ptr<vulkan::pipeline::Cache> pipelineCache() const;

    /**
     * @return the pipelines, materials and textures a draw item can name by handle
     **/
    boost::shared_ptr<vulkan::pipeline::Resources> resources() const;

    /**
     * @return set 0, where each pass's camera is written and bound from - ADR-0008
     **/
    boost::shared_ptr<vulkan::frame::FrameUniforms> frameUniforms() const;

    /**
     * @return the one-shot queue everything reaching device local memory is copied by
     **/
    boost::shared_ptr<vulkan::memory::Uploader> uploader() const;

    /**
     * The colour format of what this context draws into, which every pipeline built here is
     * built against because dynamic rendering has no render pass to take it from.
     **/
    VkFormat colourFormat() const noexcept;

    /**
     * @return the size of what this context draws into
     **/
    const VkExtent2D& extent() const noexcept;

    /**
     * The format a pipeline that depth tests has to be built against.
     *
     * Known from the device rather than from the image, so a pipeline can be built before
     * anything has asked for a depth buffer.
     **/
    VkFormat depthFormat() const noexcept;

    /**
     * The depth buffer, allocated on the first call and sized by extent() from then on.
     *
     * Lazy because a 2D app never asks: pong and tetris draw painter ordered quads and would
     * otherwise pay a full screen depth image for nothing.
     *
     * @return the buffer, which may be invalid if there is no area to allocate
     **/
    boost::shared_ptr<vulkan::frame::DepthBuffer> depth();

    /**
     * @return whether a depth buffer has been allocated, without allocating one
     **/
    bool hasDepth() const noexcept;

    /**
     * @return the batched quad primitive of ADR-0005, which every 2D thing draws through
     * @throw std::runtime_error if its pipelines cannot be created
     **/
    boost::shared_ptr<vulkan::renderer::Quad> quads();

    /**
     * @return whether a quad renderer has been built, without building one
     **/
    bool hasQuads() const noexcept;

    /**
     * @return the line primitive of ADR-0011, which every line in the engine draws through
     * @throw std::runtime_error if its pipelines cannot be created
     **/
    boost::shared_ptr<vulkan::renderer::Line> lines();

    /**
     * @return whether a line renderer has been built, without building one
     **/
    bool hasLines() const noexcept;

    /**
     * @return the world space quad primitive of ADR-0042
     * @throw std::runtime_error if its pipelines cannot be created
     **/
    boost::shared_ptr<vulkan::renderer::World> worldQuads();

    /**
     * @return whether a world quad renderer has been built, without building one
     **/
    bool hasWorldQuads() const noexcept;

 protected:
    /**
     * Say what this context draws into.
     *
     * A destination that is known when the context is built passes it here once. Context3D
     * cannot: its chain is created after this class is, and is recreated whenever the window
     * changes size, so it calls this again each time. Anything already built against the old
     * description stays built against it - a renderer holds the format it was compiled with.
     *
     * @param colour the colour format of the destination
     * @param extent its size
     **/
    void describe(VkFormat colour, const VkExtent2D& extent) noexcept;

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<vulkan::device::Device> device_;
    // before everything it paces, so that it is torn down after them - a renderer's buffers
    // may still be in flight on one of its fences
    boost::shared_ptr<vulkan::frame::Ring> ring_;
    boost::shared_ptr<vulkan::pipeline::Cache> pipelineCache_;
    boost::shared_ptr<vulkan::pipeline::Resources> resources_;
    boost::shared_ptr<vulkan::memory::Uploader> uploader_;
    boost::shared_ptr<vulkan::frame::FrameUniforms> frameUniforms_;
    boost::shared_ptr<vulkan::frame::DepthBuffer> depth_;
    VkFormat colourFormat_;
    VkExtent2D extent_;
    VkFormat depthFormat_;
    boost::shared_ptr<vulkan::renderer::Quad> quads_;
    boost::shared_ptr<vulkan::renderer::Line> lines_;
    boost::shared_ptr<vulkan::renderer::World> worldQuads_;
};
};  // namespace v3d::render::realtime

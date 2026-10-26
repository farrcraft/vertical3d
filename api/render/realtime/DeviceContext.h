/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/Textures.h>
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

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
/**
 * Everything a context owns that needs only a device: the frames in flight, the pipeline
 * cache and resources, the uploader, set 0, and the quad, line and world quad renderers.
 *
 * A DeviceContext is usable on its own. WindowContext derives from it to draw into a window, and
 * adds the swapchain and the Presenter. A context that draws into an offscreen target is a
 * DeviceContext given that target's format and size. Under dynamic rendering a pipeline is
 * built against the format of whatever it draws into, and that format is all a renderer needs
 * to know about its destination.
 *
 * Every renderer is built on the first call for it and kept. An app that draws no lines then
 * pays no pipeline compiles or vertex buffers for them. A context built before its
 * destination is described has nothing to build them against yet.
 **/
class DeviceContext {
 public:
    /**
     * @param logger
     * @param device the device everything here is built on
     * @param colour the colour format of what this draws into, and what every pipeline built
     *        here is built against. WindowContext passes VK_FORMAT_UNDEFINED, because its
     *        swapchain does not exist yet, and calls describe() later
     * @param extent the size of that destination
     * @param framesInFlight how many frames may be recorded ahead of the device
     **/
    DeviceContext(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<vulkan::device::Device>& device,
        VkFormat colour = VK_FORMAT_UNDEFINED, const VkExtent2D& extent = VkExtent2D{0, 0}, uint32_t framesInFlight = 2);

    /**
     **/
    virtual ~DeviceContext();

    /**
     * @return the device backing the context
     **/
    boost::shared_ptr<vulkan::device::Device> device() const;

    /**
     * @return the frames recorded ahead of the device. Anything keeping a resource per frame
     *         in flight indexes it by this
     **/
    boost::shared_ptr<vulkan::frame::Ring> ring() const;

    /**
     * @return the cache every pipeline built on this device is compiled against
     **/
    boost::shared_ptr<vulkan::pipeline::Cache> pipelineCache() const;

    /**
     * @return the pipelines, materials and textures a draw item can name by handle
     **/
    boost::shared_ptr<Resources> resources() const;

    /**
     * @return set 0, where each pass's camera is written and bound from
     **/
    boost::shared_ptr<vulkan::frame::FrameUniforms> frameUniforms() const;

    /**
     * @return the Uploader that copies data into device local memory
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
     * Lazy because a 2D app never asks for it. Painter ordered quads need no full screen
     * depth image.
     *
     * @return the buffer, which may be invalid if there is no area to allocate
     **/
    boost::shared_ptr<vulkan::frame::DepthBuffer> depth();

    /**
     * @return whether a depth buffer has been allocated, without allocating one
     **/
    bool hasDepth() const noexcept;

    /**
     * Where a texture is uploaded and made a material. Every renderer that samples a texture
     * gets it here, and so does an app. Built with the context, because a context that draws
     * nothing still loads textures.
     **/
    boost::shared_ptr<Textures> textures() const;

    /**
     * @return the batched quad primitive, which all 2D drawing goes through
     * @throw std::runtime_error if its pipelines cannot be created
     **/
    boost::shared_ptr<renderer::Quad> quads();

    /**
     * @return whether a quad renderer has been built, without building one
     **/
    bool hasQuads() const noexcept;

    /**
     * @return the world space line primitive, which every line in the engine draws through
     * @throw std::runtime_error if its pipelines cannot be created
     **/
    boost::shared_ptr<renderer::Line> lines();

    /**
     * @return whether a line renderer has been built, without building one
     **/
    bool hasLines() const noexcept;

    /**
     * @return the world space quad primitive
     * @throw std::runtime_error if its pipelines cannot be created
     **/
    boost::shared_ptr<renderer::World> worldQuads();

    /**
     * @return whether a world quad renderer has been built, without building one
     **/
    bool hasWorldQuads() const noexcept;

 protected:
    /**
     * Say what this context draws into.
     *
     * WindowContext's swapchain is created after the DeviceContext, and is recreated whenever the
     * window changes size, so WindowContext calls describe() each time. A renderer already built
     * keeps the format it was compiled with. A depth buffer already allocated is replaced by
     * one of the new extent, and the old one is retired through the ring.
     *
     * @param colour the colour format of the destination
     * @param extent its size
     * @throw std::runtime_error if the new depth buffer cannot be allocated
     **/
    void describe(VkFormat colour, const VkExtent2D& extent);

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<vulkan::device::Device> device_;
    // declared before everything that uses it, so it is destroyed after them. Its destructor
    // waits for the device and runs what they retired
    boost::shared_ptr<vulkan::frame::Ring> ring_;
    boost::shared_ptr<vulkan::pipeline::Cache> pipelineCache_;
    boost::shared_ptr<Resources> resources_;
    boost::shared_ptr<vulkan::memory::Uploader> uploader_;
    boost::shared_ptr<vulkan::frame::FrameUniforms> frameUniforms_;
    boost::shared_ptr<vulkan::frame::DepthBuffer> depth_;
    VkFormat colourFormat_;
    VkExtent2D extent_;
    VkFormat depthFormat_;
    boost::shared_ptr<Textures> textures_;
    boost::shared_ptr<renderer::Quad> quads_;
    boost::shared_ptr<renderer::Line> lines_;
    boost::shared_ptr<renderer::World> worldQuads_;
};
};  // namespace v3d::render::realtime

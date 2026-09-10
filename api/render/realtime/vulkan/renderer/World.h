/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/WorldCanvas.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/FrameUniforms.h>
#include <api/render/realtime/vulkan/frame/Presenter.h>
#include <api/render/realtime/vulkan/memory/Buffer.h>
#include <api/render/realtime/vulkan/pipeline/Cache.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Quad.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::renderer {

/**
 * The device half of the world space quad primitive - ADR-0042.
 *
 * A canvas is filled on the cpu during a tick and handed here, which uploads it into
 * buffers belonging to the frame about to be recorded and turns each of its batches into a
 * draw item on a pass. The buffers are per frame in flight, because the device may still be
 * reading the previous frame's out of the previous slot.
 *
 * A frame may submit any number of canvases, and each submission takes a pair of buffers of
 * its own out of the frame's ring. They cannot share one pair: growing a buffer replaces the
 * allocation, which invalidates the handle every draw item already recorded holds.
 *
 * **Textures and their descriptors are the quad renderer's.** A world quad samples through
 * the same set 1 layout as a ui quad, so an atlas uploaded once serves both and there is one
 * descriptor pool rather than two. What differs is the vertex stage, which reads the pass
 * camera at set 0 instead of pushing a projection of its own.
 **/
class World final {
 public:
    /**
     * @param logger
     * @param device the device to build the pipelines and buffers on
     * @param cache the pipeline cache every pipeline is compiled against
     * @param resources where the pipelines are registered
     * @param presenter which frame in flight is being recorded, and when its buffers are free
     * @param uniforms set 0, which both pipelines declare and read - a world canvas is in
     *        world space, so the pass's camera is its whole transform
     * @param quads where a texture becomes the set 1 descriptor a draw item names
     * @param colour the format of the image the pass draws into, which dynamic rendering
     *        needs at pipeline creation because there is no render pass to take it from
     * @param depth the format of the depth image, for the second of the two pipelines
     * @throw std::runtime_error if the pipelines cannot be created
     **/
    World(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<device::Device>& device,
        const boost::shared_ptr<pipeline::Cache>& cache, const boost::shared_ptr<pipeline::Resources>& resources,
        const boost::shared_ptr<frame::Presenter>& presenter, const boost::shared_ptr<frame::FrameUniforms>& uniforms,
        const boost::shared_ptr<Quad>& quads, VkFormat colour, VkFormat depth);

    /**
     **/
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    /**
     * Upload a canvas and add a draw item to the pass for each of its batches.
     *
     * An empty canvas costs neither an upload nor a draw.
     *
     * @param canvas the geometry to draw, which is copied and not kept
     * @param pass where the draw items are submitted. Its camera is what the quads are
     *        drawn through, so a pass that never had one set draws them in clip space
     * @param layer the painter order the items sort at
     **/
    void submit(const WorldCanvas& canvas, Pass* pass, uint16_t layer = 0);

    /**
     * Give back the buffers this frame's submissions took, so the next frame starts at the
     * front of the ring again. The engine calls this once a frame has been recorded.
     **/
    void endFrame() noexcept;

 private:
    /**
     * Compile the world quad pipeline twice - once for a pass with a depth attachment and
     * once for a pass without, because dynamic rendering matches a pipeline to the
     * attachments of the pass it draws into.
     *
     * The one built for a pass with depth **tests without writing**, per ADR-0042: solid
     * geometry in front of a quad hides it, and two blended quads do not cut holes in each
     * other where their transparent parts overlap.
     **/
    void createPipelines(VkFormat colour, VkFormat depth);

    /**
     * A canvas's geometry for one frame - one submission's worth.
     **/
    struct Geometry {
        boost::shared_ptr<memory::Buffer> vertices;
        boost::shared_ptr<memory::Buffer> indices;
    };

    /**
     * Take the next free pair of buffers of the frame being recorded, adding one to the
     * ring if every pair in it has already been claimed this frame.
     **/
    Geometry claim();

    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<pipeline::Cache> cache_;
    boost::shared_ptr<pipeline::Resources> resources_;
    boost::shared_ptr<frame::Presenter> presenter_;
    boost::shared_ptr<frame::FrameUniforms> uniforms_;
    boost::shared_ptr<Quad> quads_;

    PipelineHandle pipeline_;       /**< for a pass with no depth attachment **/
    PipelineHandle depthPipeline_;  /**< for a pass with one, and it tests without writing **/

    /**< a ring of geometry per frame in flight, grown as a frame's submissions ask **/
    std::vector<std::vector<Geometry>> geometry_;
    std::size_t cursor_;  /**< how far into the current frame's ring submit() has got **/
};

};  // namespace v3d::render::realtime::vulkan::renderer

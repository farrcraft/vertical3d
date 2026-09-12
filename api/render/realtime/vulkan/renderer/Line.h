/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/LineCanvas.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/FrameUniforms.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/memory/Buffer.h>
#include <api/render/realtime/vulkan/pipeline/Cache.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::renderer {

/**
 * The device half of the line primitive - ADR-0011.
 *
 * A canvas is filled on the cpu during a tick and handed here, which uploads it into a
 * buffer belonging to the frame about to be recorded. There is no texture, no material
 * and no index buffer, so an uncut canvas becomes one draw and a clipped one becomes a
 * draw per rectangle it is cut to. The buffers are per frame in flight, because the
 * device may still be reading the previous frame's out of the previous slot.
 *
 * A frame may submit any number of canvases, and each submission takes a buffer of its
 * own out of the frame's ring. They cannot share one: growing a buffer replaces the
 * allocation, which invalidates the handle every draw item already recorded holds.
 *
 * Lines are one pixel wide. Wider ones need the wideLines device feature, which the
 * device does not ask for.
 **/
class Line final {
 public:
    /**
     * @param logger
     * @param device the device to build the pipelines and buffers on
     * @param cache the pipeline cache every pipeline is compiled against
     * @param resources where the pipelines are registered
     * @param ring which frame in flight is being recorded, and when its buffers are free
     * @param uniforms set 0, which the line pipelines both declare and read - a line
     *        canvas is world space, so the pass's camera is its whole transform
     * @param colour the format of the image the pass draws into, which dynamic rendering
     *        needs at pipeline creation because there is no render pass to take it from
     * @param depth the format of the depth image, for the second of the two pipelines
     * @throw std::runtime_error if the pipelines cannot be created
     **/
    Line(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<device::Device>& device,
        const boost::shared_ptr<pipeline::Cache>& cache, const boost::shared_ptr<pipeline::Resources>& resources,
        const boost::shared_ptr<frame::Ring>& ring, const boost::shared_ptr<frame::FrameUniforms>& uniforms,
        VkFormat colour, VkFormat depth);

    /**
     **/
    ~Line();

    Line(const Line&) = delete;
    Line& operator=(const Line&) = delete;

    /**
     * Upload a canvas and add one draw item to the pass per batch of it.
     *
     * An empty canvas costs neither an upload nor a draw.
     *
     * @param canvas the geometry to draw, which is copied and not kept
     * @param pass where the draw item is submitted. Its camera is what the lines are
     *        drawn through, so a pass that never had one set draws them in clip space
     * @param layer the painter order the item sorts at
     **/
    void submit(const LineCanvas& canvas, Pass* pass, uint16_t layer = 0);

    /**
     * Give back the buffers this frame's submissions took, so the next frame starts at
     * the front of the ring again. The engine calls this once a frame has been recorded.
     **/
    void endFrame() noexcept;

 private:
    /**
     * Compile the line pipeline twice - once for a pass with a depth attachment and once
     * for a pass without, because dynamic rendering matches a pipeline to the attachments
     * of the pass it draws into.
     *
     * The two differ in behaviour as well as in format: the one built for a pass with
     * depth tests and writes it, so a wireframe is occluded by the geometry in front of
     * it. Lines drawn over a scene rather than into it belong in a pass with no depth.
     **/
    void createPipelines(VkFormat colour, VkFormat depth);

    /**
     * Take the next free buffer of the frame being recorded, adding one to the ring if
     * every buffer in it has already been claimed this frame.
     **/
    boost::shared_ptr<memory::Buffer> claim();

    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<pipeline::Cache> cache_;
    boost::shared_ptr<pipeline::Resources> resources_;
    boost::shared_ptr<frame::Ring> ring_;
    boost::shared_ptr<frame::FrameUniforms> uniforms_;

    PipelineHandle pipeline_;       /**< for a pass with no depth attachment **/
    PipelineHandle depthPipeline_;  /**< for a pass with one, and it tests against it **/

    /**< a ring of vertex buffers per frame in flight, grown as a frame's submissions ask **/
    std::vector<std::vector<boost::shared_ptr<memory::Buffer>>> vertices_;
    std::size_t cursor_;  /**< how far into the current frame's ring submit() has got **/
};

};  // namespace v3d::render::realtime::vulkan::renderer

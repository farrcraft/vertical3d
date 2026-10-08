/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/Canvas.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/Textures.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/FrameUniforms.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/frame/StreamRing.h>
#include <api/render/realtime/vulkan/memory/Buffer.h>
#include <api/render/realtime/vulkan/memory/TextureFactory.h>
#include <api/render/realtime/vulkan/pipeline/Cache.h>
#include <api/render/realtime/vulkan/pipeline/DescriptorPool.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>

namespace v3d::image {
class Image;
};  // namespace v3d::image

namespace v3d::render::realtime::vulkan::frame {
class RenderTarget;
};  // namespace v3d::render::realtime::vulkan::frame

namespace v3d::render::realtime::vulkan::renderer {

/**
 * The device side of the batched quad primitive.
 *
 * There is one pipeline here and every 2D thing in the engine draws with it: a rectangle,
 * a sprite and a glyph differ only in which texture is bound and what the vertex colour is.
 *
 * A Canvas is filled on the CPU during a tick and handed here. This uploads its geometry
 * into buffers belonging to the frame about to be recorded, and turns each of its batches
 * into a draw item on a pass. The buffers are per frame in flight, because the device may
 * still be reading the previous frame's out of the previous slot.
 *
 * A frame may submit any number of canvases, and each submission takes a pair of
 * buffers of its own out of the frame's ring. They cannot share one pair: growing a
 * buffer replaces the allocation, which invalidates the handle every draw item already
 * recorded holds.
 *
 * The pipelines it registers belong to pipeline::Resources and live until the context does.
 * Textures and their materials belong to the context's Textures.
 **/
class Quad final {
 public:
    /**
     * @param device the device to build the pipeline and buffers on
     * @param cache the pipeline cache every pipeline is compiled against
     * @param resources where the pipeline, textures and materials are registered
     * @param ring which frame in flight is being recorded, and when its buffers are free
     * @param uniforms set 0, whose layout the quad pipelines declare so that a pass can
     *        bind one camera across them and every other pipeline in the engine
     * @param colour the format of the image the pass draws into, which dynamic rendering
     *        needs at pipeline creation because there is no render pass to take it from
     * @param depth the format of the depth image, for the second of the two pipelines
     * @throw std::runtime_error if the pipelines or their resources cannot be created
     **/
    Quad(const boost::shared_ptr<device::Device>& device,
        const boost::shared_ptr<pipeline::Cache>& cache, const boost::shared_ptr<pipeline::Resources>& resources,
        const boost::shared_ptr<frame::Ring>& ring, const boost::shared_ptr<frame::FrameUniforms>& uniforms,
        const boost::shared_ptr<Textures>& textures, VkFormat colour, VkFormat depth);

    /**
     **/
    ~Quad();

    Quad(const Quad&) = delete;
    Quad& operator=(const Quad&) = delete;

    /**
     * Upload a canvas and add a draw item to the pass for each of its batches.
     *
     * The projection is taken from the canvas, so the caller only has to have sized it.
     * An empty canvas costs neither an upload nor a draw.
     *
     * @param canvas the geometry to draw, which is copied and not kept
     * @param pass where the draw items are submitted
     * @param layer the layer the items' sort key carries. It only has an effect in a pass
     *        that sorts; an unsorted pass records in submission order, and within one
     *        canvas submission order decides what is on top
     **/
    void submit(const Canvas& canvas, Pass* pass, uint16_t layer = 0);

    /**
     * Whether a quad drawn into this format is decoded to linear before it is written.
     *
     * A quad's colour is the colour that should appear. An _SRGB format encodes what is stored,
     * so for one of those the quad is decoded first and the encode gives the colour back. Any
     * other format stores what it is given.
     *
     * Device free, so a test can pin which formats are converted.
     **/
    static bool linearises(VkFormat colour) noexcept;

 private:
    /**
     * Compile the quad pipeline twice - once for a pass with a depth attachment and once
     * for a pass without.
     *
     * Two, because dynamic rendering matches a pipeline to the attachments of the pass it
     * draws into: one built with no depth format cannot draw into a pass that has one.
     * Neither tests or writes depth - 2D is painter ordered either way, and a ui drawn
     * over a scene has to stay on top of it whatever the scene left in the buffer.
     **/
    void createPipelines(VkFormat colour, VkFormat depth);

    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<pipeline::Cache> cache_;
    boost::shared_ptr<pipeline::Resources> resources_;
    boost::shared_ptr<frame::Ring> ring_;
    boost::shared_ptr<frame::FrameUniforms> uniforms_;
    boost::shared_ptr<Textures> textures_;

    PipelineHandle pipeline_;               /**< for a pass with no depth attachment **/
    PipelineHandle depthPipeline_;          /**< for a pass with one **/

    boost::shared_ptr<frame::StreamRing> stream_;  /**< what a frame's geometry is streamed through **/
};

};  // namespace v3d::render::realtime::vulkan::renderer

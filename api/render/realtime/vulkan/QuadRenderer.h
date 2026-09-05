/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

#include "Buffer.h"
#include "Device.h"
#include "FrameUniforms.h"
#include "PipelineCache.h"
#include "Presenter.h"
#include "Resources.h"
#include "TextureFactory.h"

#include "../Canvas.h"
#include "../Handle.h"
#include "../Pass.h"

#include "../../../log/Logger.h"

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>

namespace v3d::image {
class Image;
};  // namespace v3d::image

namespace v3d::render::realtime::vulkan {

/**
 * The device half of the one batched quad primitive - ADR-0005.
 *
 * There is one pipeline here and every 2D thing in the engine draws with it: a rectangle,
 * a sprite and a glyph differ only in which texture is bound and what the vertex colour is.
 *
 * A Canvas is filled on the cpu during a tick and handed here, which uploads its geometry
 * into buffers belonging to the frame about to be recorded and turns each of its
 * batches into a draw item on a pass. The buffers are per frame in flight, because the
 * device may still be reading the previous frame's out of the previous slot.
 *
 * A frame may submit any number of canvases, and each submission takes a pair of
 * buffers of its own out of the frame's ring. They cannot share one pair: growing a
 * buffer replaces the allocation, which invalidates the handle every draw item already
 * recorded holds.
 *
 * Everything it registers - the pipeline, the white texture, a material per texture -
 * belongs to Resources and lives until the context does.
 **/
class QuadRenderer final {
 public:
    /**
     * @param logger
     * @param device the device to build the pipeline and buffers on
     * @param cache the pipeline cache every pipeline is compiled against
     * @param resources where the pipeline, textures and materials are registered
     * @param presenter which frame in flight is being recorded, and when its buffers are free
     * @param uniforms set 0, whose layout the quad pipelines declare so that a pass can
     *        bind one camera across them and every other pipeline in the engine
     * @param colour the format of the image the pass draws into, which dynamic rendering
     *        needs at pipeline creation because there is no render pass to take it from
     * @param depth the format of the depth image, for the second of the two pipelines
     * @throw std::runtime_error if the pipelines or their resources cannot be created
     **/
    QuadRenderer(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Device>& device,
        const boost::shared_ptr<PipelineCache>& cache, const boost::shared_ptr<Resources>& resources,
        const boost::shared_ptr<Presenter>& presenter, const boost::shared_ptr<FrameUniforms>& uniforms,
        VkFormat colour, VkFormat depth);

    /**
     **/
    ~QuadRenderer();

    QuadRenderer(const QuadRenderer&) = delete;
    QuadRenderer& operator=(const QuadRenderer&) = delete;

    /**
     * Upload an image and register it, so a canvas can name it.
     * @return the handle to draw with
     **/
    TextureHandle texture(const boost::shared_ptr<v3d::image::Image>& image);

    /**
     * @param pixels tightly packed rows of width * channels bytes
     * @param channels 1 for a coverage mask such as a glyph atlas, 3 or 4 for colour
     **/
    TextureHandle texture(const unsigned char* pixels, uint32_t width, uint32_t height, uint32_t channels);

    /**
     * @return the 1x1 white texture an untextured quad is drawn against
     **/
    TextureHandle white() const noexcept;

    /**
     * Upload a canvas and add a draw item to the pass for each of its batches.
     *
     * The projection is taken from the canvas, so the caller only has to have sized it.
     * An empty canvas costs neither an upload nor a draw.
     *
     * @param canvas the geometry to draw, which is copied and not kept
     * @param pass where the draw items are submitted
     * @param layer the painter order the items sort at, for a caller drawing a ui over a
     *        game - within one canvas, submission order is what decides what is on top
     **/
    void submit(const Canvas& canvas, Pass* pass, uint16_t layer = 0);

    /**
     * Give back the buffers this frame's submissions took, so the next frame starts at
     * the front of the ring again. The engine calls this once a frame has been recorded.
     **/
    void endFrame() noexcept;

 private:
    /**
     * Build the per material descriptor set layout. Set 0's belongs to FrameUniforms,
     * because every pipeline in the engine has to declare the same one.
     **/
    void createLayouts();

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

    /**
     * A canvas's geometry for one frame - one submission's worth.
     **/
    struct Geometry {
        boost::shared_ptr<Buffer> vertices;
        boost::shared_ptr<Buffer> indices;
    };

    /**
     * Take the next free pair of buffers of the frame being recorded, adding one to the
     * ring if every pair in it has already been claimed this frame.
     **/
    Geometry claim();

    /**
     * The 1x1 white texture, so that an untextured quad needs no second pipeline.
     **/
    void createWhite();

    /**
     * The descriptor set that binds a texture at set 1, created on first use and kept.
     **/
    MaterialHandle material(const TextureHandle& texture);

    /**
     * Add a descriptor pool, because the last one is full or there is none.
     **/
    void addPool();

    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<Device> device_;
    boost::shared_ptr<PipelineCache> cache_;
    boost::shared_ptr<Resources> resources_;
    boost::shared_ptr<Presenter> presenter_;
    boost::shared_ptr<FrameUniforms> uniforms_;
    boost::shared_ptr<TextureFactory> factory_;

    VkDescriptorSetLayout materialLayout_;  /**< set 1, the sampler every quad reads through **/
    std::vector<VkDescriptorPool> pools_;
    uint32_t remaining_;                    /**< sets left in the last pool **/

    PipelineHandle pipeline_;               /**< for a pass with no depth attachment **/
    PipelineHandle depthPipeline_;          /**< for a pass with one **/
    TextureHandle white_;
    std::map<uint32_t, MaterialHandle> materials_;

    /**< a ring of geometry per frame in flight, grown as a frame's submissions ask **/
    std::vector<std::vector<Geometry>> geometry_;
    std::size_t cursor_;  /**< how far into the current frame's ring submit() has got **/
};

};  // namespace v3d::render::realtime::vulkan

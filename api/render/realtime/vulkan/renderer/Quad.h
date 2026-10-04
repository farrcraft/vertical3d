/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/Canvas.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/FrameUniforms.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
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
 * belongs to pipeline::Resources. The pipelines and the white texture live until the
 * context does; a texture lives until it is released here, along with its material.
 **/
class Quad final {
 public:
    /**
     * @param logger
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
    Quad(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<device::Device>& device,
        const boost::shared_ptr<pipeline::Cache>& cache, const boost::shared_ptr<pipeline::Resources>& resources,
        const boost::shared_ptr<frame::Ring>& ring, const boost::shared_ptr<frame::FrameUniforms>& uniforms,
        VkFormat colour, VkFormat depth);

    /**
     **/
    ~Quad();

    Quad(const Quad&) = delete;
    Quad& operator=(const Quad&) = delete;

    /**
     * Upload an image and register it, so a canvas can name it.
     * @param encoding how a shader reads it back - as authored unless something lights it
     * @return the handle to draw with
     **/
    TextureHandle texture(const boost::shared_ptr<v3d::image::Image>& image,
        memory::TextureFactory::Encoding encoding = memory::TextureFactory::Encoding::Display);

    /**
     * @param pixels tightly packed rows of width * channels bytes
     * @param channels 1 for a coverage mask such as a glyph atlas, 3 or 4 for colour
     **/
    TextureHandle texture(const unsigned char* pixels, uint32_t width, uint32_t height, uint32_t channels);

    /**
     * Register a render target so that a canvas can sample what a pass drew into it.
     *
     * What is registered shares the target's images. A target that is resized allocates
     * new ones, and the handle this returned goes on naming the old ones, which it keeps
     * alive: after a recreate(), release the old handle and register the target again.
     *
     * A target with no colour image has nothing to register, and comes back as the white
     * texture for the same reason depthTexture() gives one for a target with no depth to read.
     *
     * @param slot which of the target's images - one handle per slot for a target holding one
     *        per frame in flight, of which a reader names current() or previous() each frame
     * @return the handle to draw with
     **/
    TextureHandle texture(const frame::RenderTarget& target, uint32_t slot = 0);

    /**
     * Register a render target's depth image, so that a draw can sample what a pass tested
     * against rather than what it painted - which is the read half of a shadow map.
     *
     * The same shared contract, and the same rule about releasing and registering again
     * after a recreate(). A target built without a depth image, or with one it was not told
     * would be sampled, has nothing to register: it comes back as the white texture, because a
     * descriptor set written against an image with no sampled usage is undefined and a
     * flat white shadow map is a scene that is merely unshadowed.
     *
     * @param slot which of the target's images, as texture() takes it
     * @return the handle to draw with
     **/
    TextureHandle depthTexture(const frame::RenderTarget& target, uint32_t slot = 0);

    /**
     * @return the 1x1 white texture an untextured quad is drawn against
     **/
    TextureHandle white() const noexcept;

    /**
     * Release a texture and the material drawn with it - ADR-0061. The handle resolves to
     * nothing at once, and the image and the descriptor set are reclaimed once no frame in
     * flight can still be reading them.
     *
     * @return whether anything was released. The white texture is never released, so a
     *         handle depthTexture() gave back for a target with nothing to sample can be
     *         released like any other without taking it away from everything else.
     **/
    bool release(const TextureHandle& handle);

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

    /**
     * The descriptor set that binds a texture at set 1, created on first use and kept.
     *
     * Public because the world space quad of ADR-0042 samples through the same layout, so
     * an atlas uploaded once serves both primitives out of one descriptor pool.
     *
     * @return the material to name on a draw item, or an unset handle for a texture this
     *         does not hold
     **/
    MaterialHandle material(const TextureHandle& handle);

    /**
     * @return set 1's layout, which a second pipeline sampling a texture the same way
     *         declares so that a material allocated here is compatible with it
     **/
    VkDescriptorSetLayout materialLayout() const noexcept;

 private:
    /**
     * Build the per material descriptor set layout. Set 0's belongs to frame::FrameUniforms,
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
        boost::shared_ptr<memory::Buffer> vertices;
        boost::shared_ptr<memory::Buffer> indices;
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

    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<pipeline::Cache> cache_;
    boost::shared_ptr<pipeline::Resources> resources_;
    boost::shared_ptr<frame::Ring> ring_;
    boost::shared_ptr<frame::FrameUniforms> uniforms_;
    boost::shared_ptr<memory::TextureFactory> factory_;

    boost::shared_ptr<pipeline::DescriptorPool> materialSets_;  /**< set 1, the sampler every quad reads through **/

    PipelineHandle pipeline_;               /**< for a pass with no depth attachment **/
    PipelineHandle depthPipeline_;          /**< for a pass with one **/
    TextureHandle white_;
    /**< keyed by the whole handle, so a slot reused after a release never finds the old set **/
    std::map<TextureHandle, MaterialHandle> materials_;

    /**< a ring of geometry per frame in flight, grown as a frame's submissions ask **/
    std::vector<std::vector<Geometry>> geometry_;
    std::size_t cursor_;  /**< how far into the current frame's ring submit() has got **/
};

};  // namespace v3d::render::realtime::vulkan::renderer

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <map>
#include <vector>

#include "Buffer.h"
#include "Device.h"
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
     * into the buffers belonging to the frame about to be recorded and turns each of its
     * batches into a draw item on a pass. The buffers are per frame in flight, because the
     * device may still be reading the previous frame's out of the previous slot.
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
         * @param colour the format of the image the pass draws into, which dynamic rendering
         *        needs at pipeline creation because there is no render pass to take it from
         * @throw std::runtime_error if the pipeline or its resources cannot be created
         **/
        QuadRenderer(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Device>& device,
            const boost::shared_ptr<PipelineCache>& cache, const boost::shared_ptr<Resources>& resources,
            const boost::shared_ptr<Presenter>& presenter, VkFormat colour);

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

     private:
        /**
         * Build the descriptor set layouts and the pipeline layout the quad pipeline uses.
         **/
        void createLayouts();

        /**
         * Compile the one pipeline, against the format the pass draws into.
         **/
        void createPipeline(VkFormat colour);

        /**
         * Allocate the geometry buffers, one set per frame in flight.
         **/
        void createBuffers();

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
        boost::shared_ptr<TextureFactory> factory_;

        VkDescriptorSetLayout frameLayout_;     /**< set 0, per frame - empty until a pass has anything to bind **/
        VkDescriptorSetLayout materialLayout_;  /**< set 1, the sampler every quad reads through **/
        std::vector<VkDescriptorPool> pools_;
        uint32_t remaining_;                    /**< sets left in the last pool **/

        PipelineHandle pipeline_;
        TextureHandle white_;
        std::map<uint32_t, MaterialHandle> materials_;

        std::vector<boost::shared_ptr<Buffer>> vertices_;
        std::vector<boost::shared_ptr<Buffer>> indices_;
    };

};  // namespace v3d::render::realtime::vulkan

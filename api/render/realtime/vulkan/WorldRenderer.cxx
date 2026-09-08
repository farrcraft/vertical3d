/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "WorldRenderer.h"

#include <cstddef>
#include <vector>

#include "PipelineBuilder.h"

#include "../DrawItem.h"

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan {

namespace {

/**
 * The world quad pipeline's shader modules, compiled to SPIR-V at build time by glslc and
 * included here as the C initialiser lists its -mfmt=c writes - see v3d_add_shader in the
 * root CMakeLists.
 **/
const uint32_t vertexShader[] =
#include "shaders/world.vert.inc"
;  // NOLINT(whitespace/semicolon)

const uint32_t fragmentShader[] =
#include "shaders/world.frag.inc"
;  // NOLINT(whitespace/semicolon)

/**
 * What each buffer starts at, in bytes. They double from here when a canvas does not fit.
 **/
const VkDeviceSize initialVertexBytes = 64ULL * 1024;
const VkDeviceSize initialIndexBytes = 16ULL * 1024;

};  // namespace

/**
 **/
WorldRenderer::WorldRenderer(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Device>& device,
    const boost::shared_ptr<PipelineCache>& cache, const boost::shared_ptr<Resources>& resources,
    const boost::shared_ptr<Presenter>& presenter, const boost::shared_ptr<FrameUniforms>& uniforms,
    const boost::shared_ptr<QuadRenderer>& quads, VkFormat colour, VkFormat depth) :
    logger_(logger),
    device_(device),
    cache_(cache),
    resources_(resources),
    presenter_(presenter),
    uniforms_(uniforms),
    quads_(quads),
    cursor_(0) {
    createPipelines(colour, depth);
    geometry_.resize(presenter_->framesInFlight() > 0 ? presenter_->framesInFlight() : 1);
}

/**
 **/
WorldRenderer::~WorldRenderer() {
    // the pipelines and their layouts belong to Resources, and the buffers go with the
    // shared pointers holding them
}

/**
 **/
void WorldRenderer::createPipelines(VkFormat colour, VkFormat depth) {
    PipelineBuilder builder(device_);
    builder.name("world")
        .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
        .shader(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, sizeof(fragmentShader))
        .vertexBinding(0, sizeof(WorldCanvas::Vertex))
        .vertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(WorldCanvas::Vertex, position))
        .vertexAttribute(1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(WorldCanvas::Vertex, uv))
        .vertexAttribute(2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(WorldCanvas::Vertex, colour))
        // a quad standing in the world is seen from whichever side the camera is on, and a
        // tile highlight is seen from above and below - ADR-0042
        .cull(VK_CULL_MODE_NONE)
        .set(uniforms_->layout())
        .set(quads_->materialLayout())
        .colourFormat(colour);

    pipeline_ = resources_->add(builder.build(cache_));

    // tests and does not write, per ADR-0042: the scene occludes a quad and a quad does not
    // cut a hole in the one behind it where both are transparent
    builder.name("world-depth").depth(true, false).depthFormat(depth);
    depthPipeline_ = resources_->add(builder.build(cache_));
}

/**
 **/
WorldRenderer::Geometry WorldRenderer::claim() {
    std::vector<Geometry>& ring = geometry_[presenter_->frame()];
    if (cursor_ >= ring.size()) {
        Geometry geometry;
        geometry.vertices = boost::make_shared<Buffer>(device_, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, initialVertexBytes);
        geometry.indices = boost::make_shared<Buffer>(device_, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, initialIndexBytes);
        ring.push_back(geometry);
    }
    return ring[cursor_++];
}

/**
 **/
void WorldRenderer::endFrame() noexcept {
    cursor_ = 0;
}

/**
 **/
void WorldRenderer::submit(const WorldCanvas& canvas, Pass* pass, uint16_t layer) {
    if (pass == nullptr || canvas.empty() || !quads_) {
        return;
    }

    // the device may still be reading what this frame's slots held two frames ago
    presenter_->waitFrame();

    const Geometry claimed = claim();
    const VkDeviceSize vertexBytes = canvas.vertices().size() * sizeof(WorldCanvas::Vertex);
    const VkDeviceSize indexBytes = canvas.indices().size() * sizeof(uint32_t);

    claimed.vertices->grow(vertexBytes);
    claimed.indices->grow(indexBytes);
    claimed.vertices->write(canvas.vertices().data(), vertexBytes);
    claimed.indices->write(canvas.indices().data(), indexBytes);

    // dynamic rendering matches a pipeline to the pass's attachments, so which of the two
    // is drawn with follows from whether the pass has a depth buffer
    const PipelineHandle handle = pass->depth() ? depthPipeline_ : pipeline_;

    for (const WorldCanvas::Batch& batch : canvas.batches()) {
        if (batch.indices == 0) {
            continue;
        }
        // an unset texture is the untextured case, drawn against white
        const MaterialHandle bound =
            quads_->material(batch.texture.valid() ? batch.texture : quads_->white());

        DrawItem item;
        item.key.layer = layer;
        item.key.pipeline = static_cast<uint16_t>(handle.id());
        item.key.material = static_cast<uint16_t>(bound.id());
        item.pipeline = handle;
        item.material = bound;
        item.vertexBuffer = claimed.vertices->handle();
        item.indexBuffer = claimed.indices->handle();
        item.indexType = VK_INDEX_TYPE_UINT32;
        item.indices = batch.indices;
        item.firstIndex = batch.firstIndex;
        item.instances = 1;

        pass->submit(item);
    }
}

};  // namespace v3d::render::realtime::vulkan

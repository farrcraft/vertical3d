/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "LineRenderer.h"

#include <cstddef>
#include <vector>

#include "PipelineBuilder.h"

#include "../DrawItem.h"

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan {

namespace {

/**
 * The line pipeline's shader modules, compiled to SPIR-V at build time by glslc and
 * included here as the C initialiser lists its -mfmt=c writes - see v3d_add_shader in
 * the root CMakeLists.
 **/
const uint32_t vertexShader[] =
#include "shaders/line.vert.inc"
;  // NOLINT(whitespace/semicolon)

const uint32_t fragmentShader[] =
#include "shaders/line.frag.inc"
;  // NOLINT(whitespace/semicolon)

/**
 * What each vertex buffer starts at, in bytes. The buffers double from here when a
 * canvas does not fit.
 **/
const VkDeviceSize initialVertexBytes = 64 * 1024;

};  // namespace

/**
 **/
LineRenderer::LineRenderer(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Device>& device,
    const boost::shared_ptr<PipelineCache>& cache, const boost::shared_ptr<Resources>& resources,
    const boost::shared_ptr<Presenter>& presenter, const boost::shared_ptr<FrameUniforms>& uniforms,
    VkFormat colour, VkFormat depth) :
    logger_(logger),
    device_(device),
    cache_(cache),
    resources_(resources),
    presenter_(presenter),
    uniforms_(uniforms),
    cursor_(0) {
    createPipelines(colour, depth);
    vertices_.resize(presenter_->framesInFlight() > 0 ? presenter_->framesInFlight() : 1);
}

/**
 **/
LineRenderer::~LineRenderer() {
    // the pipelines and their layouts belong to Resources, and the buffers go with the
    // shared pointers holding them
}

/**
 **/
void LineRenderer::createPipelines(VkFormat colour, VkFormat depth) {
    PipelineBuilder builder(device_);
    builder.name("line")
        .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
        .shader(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, sizeof(fragmentShader))
        .vertexBinding(0, sizeof(LineCanvas::Vertex))
        .vertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(LineCanvas::Vertex, position))
        .vertexAttribute(1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(LineCanvas::Vertex, colour))
        .topology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
        // a line has no facing to cull by
        .cull(VK_CULL_MODE_NONE)
        .set(uniforms_->layout())
        .colourFormat(colour);

    pipeline_ = resources_->add(builder.build(cache_));

    builder.name("line-depth").depth(true, true).depthFormat(depth);
    depthPipeline_ = resources_->add(builder.build(cache_));
}

/**
 **/
boost::shared_ptr<Buffer> LineRenderer::claim() {
    std::vector<boost::shared_ptr<Buffer>>& ring = vertices_[presenter_->frame()];
    if (cursor_ >= ring.size()) {
        ring.push_back(boost::make_shared<Buffer>(device_, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, initialVertexBytes));
    }
    return ring[cursor_++];
}

/**
 **/
void LineRenderer::endFrame() noexcept {
    cursor_ = 0;
}

/**
 **/
void LineRenderer::submit(const LineCanvas& canvas, Pass* pass, uint16_t layer) {
    if (pass == nullptr || canvas.empty()) {
        return;
    }

    // the device may still be reading what this frame's slots held two frames ago
    presenter_->waitFrame();

    const boost::shared_ptr<Buffer> vertices = claim();
    const VkDeviceSize vertexBytes = canvas.vertices().size() * sizeof(LineCanvas::Vertex);

    vertices->grow(vertexBytes);
    vertices->write(canvas.vertices().data(), vertexBytes);

    // dynamic rendering matches a pipeline to the pass's attachments, so which of the two
    // is drawn with follows from whether the pass has a depth buffer
    const PipelineHandle handle = pass->depth() ? depthPipeline_ : pipeline_;

    DrawItem item;
    item.key.layer = layer;
    item.key.pipeline = static_cast<uint16_t>(handle.id());
    item.pipeline = handle;
    item.vertexBuffer = vertices->handle();
    // a line list is not indexed
    item.vertices = static_cast<uint32_t>(canvas.vertices().size());
    item.instances = 1;

    pass->submit(item);
}

};  // namespace v3d::render::realtime::vulkan

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Line.h"

#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/vulkan/pipeline/Builder.h>

#include <algorithm>
#include <cstddef>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::renderer {

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
const VkDeviceSize initialVertexBytes = 64ULL * 1024;

};  // namespace

/**
 **/
Line::Line(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<device::Device>& device,
    const boost::shared_ptr<pipeline::Cache>& cache, const boost::shared_ptr<pipeline::Resources>& resources,
    const boost::shared_ptr<frame::Presenter>& presenter, const boost::shared_ptr<frame::FrameUniforms>& uniforms,
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
Line::~Line() {
    // the pipelines and their layouts belong to pipeline::Resources, and the buffers go with the
    // shared pointers holding them
}

/**
 **/
void Line::createPipelines(VkFormat colour, VkFormat depth) {
    pipeline::Builder builder(device_);
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
boost::shared_ptr<memory::Buffer> Line::claim() {
    std::vector<boost::shared_ptr<memory::Buffer>>& ring = vertices_[presenter_->frame()];
    if (cursor_ >= ring.size()) {
        ring.push_back(boost::make_shared<memory::Buffer>(device_, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, initialVertexBytes));
    }
    return ring[cursor_++];
}

/**
 **/
void Line::endFrame() noexcept {
    cursor_ = 0;
}

/**
 **/
void Line::submit(const LineCanvas& canvas, Pass* pass, uint16_t layer) {
    if (pass == nullptr || canvas.empty()) {
        return;
    }

    // the device may still be reading what this frame's slots held two frames ago
    presenter_->waitFrame();

    const boost::shared_ptr<memory::Buffer> vertices = claim();
    const VkDeviceSize vertexBytes = canvas.vertices().size() * sizeof(LineCanvas::Vertex);

    vertices->grow(vertexBytes);
    vertices->write(canvas.vertices().data(), vertexBytes);

    // dynamic rendering matches a pipeline to the pass's attachments, so which of the two
    // is drawn with follows from whether the pass has a depth buffer
    const PipelineHandle handle = pass->depth() ? depthPipeline_ : pipeline_;

    for (const LineCanvas::Batch& batch : canvas.batches()) {
        if (batch.vertices == 0) {
            continue;
        }

        DrawItem item;
        item.key.layer = layer;
        item.key.pipeline = static_cast<uint16_t>(handle.id());
        item.pipeline = handle;
        item.vertexBuffer = vertices->handle();
        // a line list is not indexed
        item.vertices = batch.vertices;
        item.firstVertex = batch.firstVertex;
        item.instances = 1;

        if (batch.clipped) {
            // the canvas clips in the image's pixels already, since a world space stream
            // has no transform that would carry a rectangle to the screen - ADR-0037
            const float left = std::max(batch.clip.x, 0.0f);
            const float top = std::max(batch.clip.y, 0.0f);
            item.scissored = true;
            item.scissor.offset.x = static_cast<int32_t>(left);
            item.scissor.offset.y = static_cast<int32_t>(top);
            item.scissor.extent.width = static_cast<uint32_t>(std::max(batch.clip.z - left, 0.0f));
            item.scissor.extent.height = static_cast<uint32_t>(std::max(batch.clip.w - top, 0.0f));
        }

        pass->submit(item);
    }
}

};  // namespace v3d::render::realtime::vulkan::renderer

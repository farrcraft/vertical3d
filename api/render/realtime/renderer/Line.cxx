/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Line.h"

#include "Clip.h"

#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/canvas/Line.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/FrameUniforms.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/frame/StreamRing.h>
#include <api/render/realtime/vulkan/memory/Buffer.h>
#include <api/render/realtime/vulkan/pipeline/Builder.h>
#include <api/render/realtime/vulkan/pipeline/Cache.h>
#include <api/render/realtime/vulkan/pipeline/Formats.h>
#include <api/render/realtime/Resources.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::renderer {

namespace {

/**
 * The line pipeline's shader modules, compiled to SPIR-V at build time by glslc and
 * included here as the C initialiser lists its -mfmt=c writes, by v3d_add_shader in
 * cmake/v3dHelpers.cmake.
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
Line::Line(const boost::shared_ptr<vulkan::device::Device>& device,
    const boost::shared_ptr<vulkan::pipeline::Cache>& cache, const boost::shared_ptr<Resources>& resources,
    const boost::shared_ptr<vulkan::frame::Ring>& ring, const boost::shared_ptr<vulkan::frame::FrameUniforms>& uniforms,
    VkFormat colour, VkFormat depth) :
    device_(device),
    cache_(cache),
    resources_(resources),
    ring_(ring),
    uniforms_(uniforms) {
    stream_ = boost::make_shared<vulkan::frame::StreamRing>(device_, ring_, vulkan::frame::StreamRing::Sizes{.vertexBytes = initialVertexBytes, .indexBytes = 0});
    createPipelines({.colour = colour, .depth = depth});
}

/**
 **/
Line::~Line() {
    // the pipelines and their layouts belong to Resources, and the buffers go with the
    // shared pointers holding them
}

/**
 **/
void Line::createPipelines(const vulkan::pipeline::Formats & formats) {
    const VkFormat colour = formats.colour;
    const VkFormat depth = formats.depth;
    vulkan::pipeline::Builder builder(device_);
    builder.name("line")
        .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
        .shader(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, sizeof(fragmentShader))
        .vertexBinding({.binding = 0, .stride = sizeof(canvas::Line::Vertex)})
        .vertexAttribute({.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(canvas::Line::Vertex, position)})
        .vertexAttribute({.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(canvas::Line::Vertex, colour)})
        .topology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
        // a line has no facing to cull by
        .cull(VK_CULL_MODE_NONE)
        .set(uniforms_->layout())
        .colourFormat(colour);

    pipeline_ = resources_->add(builder.build(cache_));

    // writes depth unless the pass says otherwise, so a near wireframe hides a far one
    builder.name("line-depth").depth(true, true).depthWriteDynamic(true).depthFormat(depth);
    depthPipeline_ = resources_->add(builder.build(cache_));
}

/**
 **/
void Line::submit(const canvas::Line& canvas, Pass* pass, uint16_t layer) {
    if (pass == nullptr || canvas.empty()) {
        return;
    }

    const VkDeviceSize vertexBytes = canvas.vertices().size() * sizeof(canvas::Line::Vertex);
    const boost::shared_ptr<vulkan::memory::Buffer> vertices = stream_->claim({.vertexBytes = vertexBytes, .indexBytes = 0}).vertices;
    vertices->write(canvas.vertices().data(), vertexBytes);

    // dynamic rendering matches a pipeline to the pass's attachments, so which of the two
    // is drawn with follows from whether the pass has a depth buffer
    const PipelineHandle handle = pass->depth() ? depthPipeline_ : pipeline_;

    for (const canvas::Line::Batch& batch : canvas.batches()) {
        if (batch.vertices == 0) {
            continue;
        }

        DrawItem item;
        item.key.layer = layer;
        item.pipeline = handle;
        item.vertexBuffer = vertices->handle();
        // a line list is not indexed
        item.vertices = batch.vertices;
        item.firstVertex = batch.firstVertex;
        item.instances = 1;

        if (batch.clipped) {
            // the canvas clips in the image's pixels already, since a world space stream
            // has no transform that would carry a rectangle to the screen
            clip(&item, batch.clip);
        }

        pass->submit(item);
    }
}

};  // namespace v3d::render::realtime::renderer

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Quad.h"

#include "Clip.h"

#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/Textures.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/FrameUniforms.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/frame/StreamRing.h>
#include <api/render/realtime/vulkan/memory/Buffer.h>
#include <api/render/realtime/vulkan/pipeline/Builder.h>
#include <api/render/realtime/vulkan/pipeline/Cache.h>
#include <api/render/realtime/vulkan/pipeline/Formats.h>
#include <api/render/realtime/vulkan/pipeline/Pipeline.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::renderer {

namespace {

/**
 * The quad pipeline's shader modules, compiled to SPIR-V at build time by glslc and
 * included here as the C initialiser lists its -mfmt=c writes, by v3d_add_shader in
 * cmake/v3dHelpers.cmake.
 **/
const uint32_t vertexShader[] =
#include "shaders/quad.vert.inc"
;  // NOLINT(whitespace/semicolon)

const uint32_t fragmentShader[] =
#include "shaders/quad.frag.inc"
;  // NOLINT(whitespace/semicolon)

const uint32_t linearFragmentShader[] =
#include "shaders/linear_quad.frag.inc"
;  // NOLINT(whitespace/semicolon)

/**
 * What each geometry buffer starts at, in bytes. A screen of quads fits without
 * growing, and the buffers double from here when something does not.
 **/
const VkDeviceSize initialVertexBytes = 64ULL * 1024;
const VkDeviceSize initialIndexBytes = 32ULL * 1024;

/**
 * The push constant block, laid out as quad.vert and quad.frag declare it.
 *
 * One range covers both stages: the projection is the vertex stage's and text is the
 * fragment stage's, and a batch is one or the other for all of its fragments.
 **/
struct Push final {
    glm::mat4 projection;
    uint32_t text;
};

};  // namespace

/**
 **/
Quad::Quad(const boost::shared_ptr<device::Device>& device,
    const boost::shared_ptr<pipeline::Cache>& cache, const boost::shared_ptr<pipeline::Resources>& resources,
    const boost::shared_ptr<frame::Ring>& ring, const boost::shared_ptr<frame::FrameUniforms>& uniforms,
    const boost::shared_ptr<Textures>& textures, VkFormat colour, VkFormat depth) :
    device_(device),
    cache_(cache),
    resources_(resources),
    ring_(ring),
    uniforms_(uniforms),
    textures_(textures) {
    stream_ = boost::make_shared<frame::StreamRing>(device_, ring_, frame::StreamRing::Sizes{.vertexBytes = initialVertexBytes, .indexBytes = initialIndexBytes});
    createPipelines({.colour = colour, .depth = depth});
}

/**
 **/
Quad::~Quad() {
    // the pipelines and their layouts belong to pipeline::Resources and the textures to the
    // context's Textures. The geometry buffers are owned here and are destroyed with it
}

/**
 **/
bool Quad::linearises(VkFormat colour) noexcept {
    switch (colour) {
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_R8G8_SRGB:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_SRGB:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
            return true;
        default:
            return false;
    }
}

/**
 **/
void Quad::createPipelines(const pipeline::Formats & formats) {
    const VkFormat colour = formats.colour;
    const VkFormat depth = formats.depth;
    const bool linear = linearises(colour);
    pipeline::Builder builder(device_);
    builder.name("quad")
        .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
        .shader(VK_SHADER_STAGE_FRAGMENT_BIT, linear ? linearFragmentShader : fragmentShader,
            linear ? sizeof(linearFragmentShader) : sizeof(fragmentShader))
        .vertexBinding({.binding = 0, .stride = sizeof(canvas::Canvas::Vertex)})
        .vertexAttribute({.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(canvas::Canvas::Vertex, position)})
        .vertexAttribute({.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(canvas::Canvas::Vertex, uv)})
        .vertexAttribute({.location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(canvas::Canvas::Vertex, colour)})
        // nothing 2D has a back face worth culling, and not culling means a caller cannot
        // get a quad's winding wrong and have it silently disappear
        .cull(VK_CULL_MODE_NONE)
        .set(uniforms_->layout())
        .set(textures_->layout())
        .push({.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, .size = sizeof(Push)})
        .colourFormat(colour);

    pipeline_ = resources_->add(builder.build(cache_));

    builder.name("quad-depth").depthFormat(depth);
    depthPipeline_ = resources_->add(builder.build(cache_));
}

/**
 **/
void Quad::submit(const canvas::Canvas& canvas, Pass* pass, uint16_t layer) {
    if (pass == nullptr || canvas.empty()) {
        return;
    }

    const VkDeviceSize vertexBytes = canvas.vertices().size() * sizeof(canvas::Canvas::Vertex);
    const VkDeviceSize indexBytes = canvas.indices().size() * sizeof(uint32_t);
    const frame::StreamRing::Geometry claimed = stream_->claim({.vertexBytes = vertexBytes, .indexBytes = indexBytes});
    const boost::shared_ptr<memory::Buffer>& vertices = claimed.vertices;
    const boost::shared_ptr<memory::Buffer>& indices = claimed.indices;

    vertices->write(canvas.vertices().data(), vertexBytes);
    indices->write(canvas.indices().data(), indexBytes);

    const glm::mat4 projection = canvas.projection();

    // dynamic rendering matches a pipeline to the pass's attachments, so which of the two
    // is drawn with follows from whether the pass has a depth buffer
    const PipelineHandle handle = pass->depth() ? depthPipeline_ : pipeline_;
    const pipeline::Pipeline* pipeline = resources_->pipeline(handle);
    for (const canvas::Canvas::Batch& batch : canvas.batches()) {
        if (batch.indices == 0) {
            continue;
        }
        // an unset texture is the untextured case, drawn against white
        const MaterialHandle bound = textures_->material(batch.texture.valid() ? batch.texture : textures_->white());

        DrawItem item;
        item.key.layer = layer;
        item.pipeline = handle;
        item.material = bound;
        item.vertexBuffer = vertices->handle();
        item.indexBuffer = indices->handle();
        item.indexType = VK_INDEX_TYPE_UINT32;
        item.indices = batch.indices;
        item.firstIndex = batch.firstIndex;
        item.instances = 1;

        if (batch.clipped) {
            // the canvas clips in its own pixels, which are the image's because the ui is
            // drawn into a pass covering the whole image
            clip(&item, batch.clip);
        }

        if (pipeline != nullptr && pipeline->pushStages != 0) {
            Push constants;
            constants.projection = projection;
            constants.text = batch.text ? 1U : 0U;
            std::memcpy(item.push.data(), &constants, sizeof(constants));
            item.pushSize = sizeof(constants);
        }

        pass->submit(item);
    }
}

};  // namespace v3d::render::realtime::vulkan::renderer

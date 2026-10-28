/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "World.h"

#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/Resources.h>
#include <api/render/realtime/Textures.h>
#include <api/render/realtime/canvas/World.h>
#include <api/render/realtime/vulkan/frame/FrameUniforms.h>
#include <api/render/realtime/vulkan/frame/StreamRing.h>
#include <api/render/realtime/vulkan/pipeline/Builder.h>
#include <api/render/realtime/vulkan/pipeline/Formats.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>

#include <glm/vec4.hpp>

namespace v3d::render::realtime::renderer {

namespace {

/**
 * The world quad pipeline's shader modules, compiled to SPIR-V at build time by glslc and
 * included here as the C initialiser lists its -mfmt=c writes, by v3d_add_shader in
 * cmake/v3dHelpers.cmake.
 **/
const uint32_t vertexShader[] =
#include "shaders/world.vert.inc"
;  // NOLINT(whitespace/semicolon)

const uint32_t fragmentShader[] =
#include "shaders/world.frag.inc"
;  // NOLINT(whitespace/semicolon)

/**
 * The push constant block, laid out as world.vert and world.frag declare it. The tint is the
 * vertex stage's and the texture's slot the fragment stage's.
 **/
struct Push final {
    glm::vec4 tint;
    uint32_t texture;
};

/**
 * What each buffer starts at, in bytes. They double from here when a canvas does not fit.
 **/
const VkDeviceSize initialVertexBytes = 64ULL * 1024;
const VkDeviceSize initialIndexBytes = 16ULL * 1024;

};  // namespace

/**
 **/
World::World(const DeviceContext& context, const Spec& spec) :
    device_(context.device()),
    cache_(context.pipelineCache()),
    resources_(context.resources()),
    ring_(context.ring()),
    uniforms_(context.frameUniforms()),
    textures_(context.textures()) {
    stream_ = boost::make_shared<vulkan::frame::StreamRing>(device_, ring_, vulkan::frame::StreamRing::Sizes{.vertexBytes = initialVertexBytes, .indexBytes = initialIndexBytes});
    createPipelines({.colour = spec.colour, .depth = spec.depth});
}

/**
 **/
World::~World() {
    // the pipelines and their layouts belong to Resources, and the buffers go with the
    // shared pointers holding them
}

/**
 **/
void World::createPipelines(const vulkan::pipeline::Formats & formats) {
    const vulkan::pipeline::Formats flat{.colour = formats.colour};
    pipeline_ = createPipeline("world", flat, Blend::Alpha);
    depthPipeline_ = createPipeline("world-depth", formats, Blend::Alpha);
    additivePipeline_ = createPipeline("world-additive", flat, Blend::Additive);
    additiveDepthPipeline_ = createPipeline("world-additive-depth", formats, Blend::Additive);
}

/**
 **/
PipelineHandle World::createPipeline(const std::string& name, const vulkan::pipeline::Formats & formats, Blend blend) {
    const VkFormat colour = formats.colour;
    const VkFormat depth = formats.depth;
    vulkan::pipeline::Builder builder(device_);
    builder.name(name)
        .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
        .shader(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, sizeof(fragmentShader))
        .vertexBinding({.binding = 0, .stride = sizeof(canvas::World::Vertex)})
        .vertexAttribute({.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(canvas::World::Vertex, position)})
        .vertexAttribute({.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(canvas::World::Vertex, uv)})
        .vertexAttribute({.location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(canvas::World::Vertex, colour)})
        // a quad standing in the world is seen from whichever side the camera is on, and a
        // tile highlight is seen from above and below
        .cull(VK_CULL_MODE_NONE)
        .set(uniforms_->layout())
        .set(textures_->layout())
        .push({.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, .size = sizeof(Push)})
        .colourFormat(colour);

    if (blend == Blend::Additive) {
        // the destination's alpha is kept, so adding light never changes how opaque the
        // picture is
        vulkan::pipeline::Builder::Blend adding;
        adding.sourceColour = VK_BLEND_FACTOR_SRC_ALPHA;
        adding.destinationColour = VK_BLEND_FACTOR_ONE;
        adding.sourceAlpha = VK_BLEND_FACTOR_ZERO;
        adding.destinationAlpha = VK_BLEND_FACTOR_ONE;
        builder.blend(adding);
    }

    // tests, and does not write unless the pass says otherwise. The scene occludes a quad,
    // and a quad does not cut a hole in the one behind it where both are transparent
    if (depth != VK_FORMAT_UNDEFINED) {
        builder.depth(true, false).depthWriteDynamic(true).depthFormat(depth);
    }
    return resources_->add(builder.build(cache_));
}

/**
 **/
void World::submit(const canvas::World& canvas, Pass* pass, uint16_t layer, Blend blend, const glm::vec4& tint) {
    if (pass == nullptr || canvas.empty() || !textures_) {
        return;
    }

    const VkDeviceSize vertexBytes = canvas.vertices().size() * sizeof(canvas::World::Vertex);
    const VkDeviceSize indexBytes = canvas.indices().size() * sizeof(uint32_t);
    const vulkan::frame::StreamRing::Geometry claimed = stream_->claim({.vertexBytes = vertexBytes, .indexBytes = indexBytes});
    claimed.vertices->write(canvas.vertices().data(), vertexBytes);
    claimed.indices->write(canvas.indices().data(), indexBytes);

    // dynamic rendering matches a pipeline to the pass's attachments, so which of the two
    // is drawn with follows from whether the pass has a depth buffer
    PipelineHandle handle = pass->depth() ? depthPipeline_ : pipeline_;
    if (blend == Blend::Additive) {
        handle = pass->depth() ? additiveDepthPipeline_ : additivePipeline_;
    }

    for (const canvas::World::Batch& batch : canvas.batches()) {
        if (batch.indices == 0) {
            continue;
        }
        // an unset texture is the untextured case, drawn against white
        const Push constants{.tint = tint, .texture = textures_->slot(batch.texture)};

        DrawItem item;
        item.key.layer = layer;
        item.pipeline = handle;
        item.material = textures_->material();
        item.vertexBuffer = claimed.vertices->handle();
        item.indexBuffer = claimed.indices->handle();
        item.indexType = VK_INDEX_TYPE_UINT32;
        item.indices = batch.indices;
        item.firstIndex = batch.firstIndex;
        item.instances = 1;
        std::memcpy(item.push.data(), &constants, sizeof(constants));
        item.pushSize = sizeof(constants);

        pass->submit(item);
    }
}

};  // namespace v3d::render::realtime::renderer

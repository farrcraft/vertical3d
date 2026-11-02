/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "World.h"

#include "Quad.h"

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

// the fragment stage for an _SRGB target, which corrects text's coverage to its display weight
const uint32_t encodedFragmentShader[] =
#include "shaders/encoded_world.frag.inc"
;  // NOLINT(whitespace/semicolon)

/**
 * How world.frag draws a batch, as its push constants name it.
 **/
constexpr uint32_t PICTURE = 0;
constexpr uint32_t CURVES = 1;
constexpr uint32_t DISTANCE = 2;

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
    const bool encoded = Quad::linearises(colour);
    vulkan::pipeline::Builder builder(device_);
    builder.name(name)
        .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
        .shader(VK_SHADER_STAGE_FRAGMENT_BIT, encoded ? encodedFragmentShader : fragmentShader,
            encoded ? sizeof(encodedFragmentShader) : sizeof(fragmentShader))
        .vertexBinding({.binding = 0, .stride = sizeof(canvas::World::Vertex)})
        .vertexAttribute({.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(canvas::World::Vertex, position)})
        .vertexAttribute({.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(canvas::World::Vertex, uv)})
        .vertexAttribute({.location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(canvas::World::Vertex, colour)})
        .vertexAttribute({.location = 3, .binding = 0, .format = VK_FORMAT_R32_UINT, .offset = offsetof(canvas::World::Vertex, glyph)})
        .vertexAttribute({.location = 4, .binding = 0, .format = VK_FORMAT_R32_SFLOAT, .offset = offsetof(canvas::World::Vertex, contrast)})
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
        // a run with bands draws curves, and the shader takes the curves from its texture
        uint32_t mode = batch.bands.valid() ? CURVES : PICTURE;
        if (batch.field) {
            mode = DISTANCE;
        }
        const Push constants{.tint = tint, .texture = textures_->slot(batch.texture), .bands = batch.bands.valid() ? textures_->slot(batch.bands) : 0U,
            .mode = mode};

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

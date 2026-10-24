/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "World.h"

#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/Textures.h>
#include <api/render/realtime/canvas/World.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/FrameUniforms.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/frame/StreamRing.h>
#include <api/render/realtime/vulkan/pipeline/Builder.h>
#include <api/render/realtime/vulkan/pipeline/Cache.h>
#include <api/render/realtime/vulkan/pipeline/Formats.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>

#include <glm/vec4.hpp>

namespace v3d::render::realtime::vulkan::renderer {

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
 * The push constant block, laid out as world.vert declares it.
 **/
struct Push final {
    glm::vec4 tint;
};

/**
 * What each buffer starts at, in bytes. They double from here when a canvas does not fit.
 **/
const VkDeviceSize initialVertexBytes = 64ULL * 1024;
const VkDeviceSize initialIndexBytes = 16ULL * 1024;

};  // namespace

/**
 **/
World::World(const boost::shared_ptr<device::Device>& device,
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
World::~World() {
    // the pipelines and their layouts belong to pipeline::Resources, and the buffers go with the
    // shared pointers holding them
}

/**
 **/
void World::createPipelines(const pipeline::Formats & formats) {
    const pipeline::Formats flat{.colour = formats.colour};
    pipeline_ = createPipeline("world", flat, Blend::Alpha);
    depthPipeline_ = createPipeline("world-depth", formats, Blend::Alpha);
    additivePipeline_ = createPipeline("world-additive", flat, Blend::Additive);
    additiveDepthPipeline_ = createPipeline("world-additive-depth", formats, Blend::Additive);
}

/**
 **/
PipelineHandle World::createPipeline(const std::string& name, const pipeline::Formats & formats, Blend blend) {
    const VkFormat colour = formats.colour;
    const VkFormat depth = formats.depth;
    pipeline::Builder builder(device_);
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
        .push({.stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .size = sizeof(Push)})
        .colourFormat(colour);

    if (blend == Blend::Additive) {
        // the destination's alpha is kept, so adding light never changes how opaque the
        // picture is
        pipeline::Builder::Blend adding;
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
    const frame::StreamRing::Geometry claimed = stream_->claim({.vertexBytes = vertexBytes, .indexBytes = indexBytes});
    claimed.vertices->write(canvas.vertices().data(), vertexBytes);
    claimed.indices->write(canvas.indices().data(), indexBytes);

    // dynamic rendering matches a pipeline to the pass's attachments, so which of the two
    // is drawn with follows from whether the pass has a depth buffer
    PipelineHandle handle = pass->depth() ? depthPipeline_ : pipeline_;
    if (blend == Blend::Additive) {
        handle = pass->depth() ? additiveDepthPipeline_ : additivePipeline_;
    }

    const Push constants{.tint = tint};
    for (const canvas::World::Batch& batch : canvas.batches()) {
        if (batch.indices == 0) {
            continue;
        }
        // an unset texture is the untextured case, drawn against white
        const MaterialHandle bound =
            textures_->material(batch.texture.valid() ? batch.texture : textures_->white());

        DrawItem item;
        item.key.layer = layer;
        item.pipeline = handle;
        item.material = bound;
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

};  // namespace v3d::render::realtime::vulkan::renderer

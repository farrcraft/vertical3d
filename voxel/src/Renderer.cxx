/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <api/render/realtime/vulkan/pipeline/Builder.h>
#include <api/render/realtime/vulkan/pipeline/Material.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>
#include <voxel/src/engine/Camera.h>
#include <voxel/src/engine/ChunkMeshBuilder.h>
#include <voxel/src/engine/ChunkVertex.h>
#include <voxel/src/engine/SceneUniforms.h>
#include <voxel/src/game/Player.h>
#include <voxel/src/voxel/ChunkCulling.h>
#include <voxel/src/voxel/ChunkMeshPool.h>
#include <voxel/src/voxel/MeshBuilder.h>

#include <cstddef>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Scene.h"
#include "Version.h"

#include <boost/make_shared.hpp>
#include <glm/vec3.hpp>

namespace {

/**
 * The terrain pipeline's shader modules, compiled to SPIR-V at build time by glslc and
 * included as the C initialiser lists its -mfmt=c writes - see v3d_add_shader.
 **/
const uint32_t vertexShader[] =
#include "shaders/voxel.vert.inc"
;  // NOLINT(whitespace/semicolon)

const uint32_t fragmentShader[] =
#include "shaders/voxel.frag.inc"
;  // NOLINT(whitespace/semicolon)

/**
 * The pass the terrain draws into. It is the frame's first, so it is the one that clears.
 **/
const char* const terrainPass = v3d::render::realtime::Engine3D::colourPass;

/**
 * The pass the overlay and the ui draw into, over whatever the terrain left.
 **/
const char* const overlayPass = "overlay";

/**
 * The size the ui and the debug overlay are drawn at, which the one atlas is scaled to per
 * ADR-0036 rather than rasterized at.
 **/
/**
 * What the debug readout's window is titled, which is also the id the layer knows it by.
 **/
const char* const debugTitle = "Debug";

const float fontSize = 18.0f;

constexpr glm::vec4 sky(0.4f, 0.6f, 0.9f, 1.0f);
constexpr glm::vec4 textColour(0.95f, 0.95f, 0.95f, 1.0f);
constexpr glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);

/**
 * How many chunks are remeshed in one tick. Meshing waits for its staging copy, so the
 * whole world arriving in one frame would be a visible stall.
 **/
const size_t chunkUpdatesPerTick = 16;

/**
 * The block palette, indexed by Voxel::BlockType less one - air is never meshed, so the
 * table starts at dirt.
 **/
constexpr glm::vec3 palette[materialCount] = {
    glm::vec3(0.9f, 0.5f, 0.3f),     // dirt
    glm::vec3(0.13f, 0.56f, 0.19f),  // grass
    glm::vec3(0.9f, 0.88f, 0.58f),   // sand
    glm::vec3(0.75f, 0.75f, 0.75f),  // stone
    glm::vec3(0.52f, 0.52f, 0.52f),  // gravel
    glm::vec3(0.25f, 0.39f, 0.96f),  // water
    glm::vec3(0.16f, 0.16f, 0.16f),  // ore
    glm::vec3(0.5f, 0.29f, 0.02f),   // wood
    glm::vec3(1.0f, 0.47f, 0.12f),   // lava
    glm::vec3(0.76f, 0.87f, 1.0f),   // glass
    glm::vec3(0.29f, 0.29f, 0.29f),  // bedrock
    glm::vec3(0.86f, 0.86f, 0.86f),  // clay
    glm::vec3(0.96f, 0.96f, 0.96f),  // ice
    glm::vec3(0.85f, 0.81f, 0.56f),  // sandstone
    glm::vec3(0.0f, 0.0f, 0.0f),     // obsidian
    glm::vec3(0.91f, 0.91f, 0.91f)   // snow
};

};  // namespace

/**
 **/
Renderer::Renderer(const boost::shared_ptr<Scene> & scene, const boost::shared_ptr<v3d::render::realtime::Window>& window,
    const boost::shared_ptr<v3d::log::Logger> & logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry) :
    scene_(scene),
    logger_(logger),
    engine_(logger, assetManager, registry),
    drawnChunks_(0),
    meshedChunks_(0),
    debug_(false) {
    engine_.initialize(window);
    engine_.clearColour(sky);

    context_ = boost::dynamic_pointer_cast<v3d::render::realtime::DeviceContext>(engine_.context());
    if (!context_) {
        throw std::runtime_error("The voxel renderer needs a context on a device to build its pipeline against");
    }

    createLayout();
    createUniforms();
    createPipeline();
    meshes_ = boost::make_shared<ChunkMeshPool>();
    builder_ = boost::make_shared<MeshBuilder>(scene_->chunks(),
        ChunkMeshBuilder(context_->device(), context_->uploader()));

    v3d::ui::shell::Screen::Options options;
    options.size = fontSize;
    // the frame time is in the debug readout, so there is no overlay to draw it
    options.statistics = false;
    // the debug readout is a panel written as calls rather than a tree kept in step with
    // what it shows, per ADR-0035 - it is a function of the frame it is drawn in
    options.immediate = true;
    screen_ = boost::make_shared<v3d::ui::shell::Screen>(&engine_, assetManager, logger, options);
}

/**
 **/
Renderer::~Renderer() = default;

/**
 **/
void Renderer::createLayout() {
    VkDescriptorSetLayoutBinding block{};
    block.binding = 0;
    block.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    block.descriptorCount = 1;
    // the shading is worked out per vertex, so only the vertex stage reads the palette
    block.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // one set, for the one material every chunk in the world draws with
    scenePool_ = boost::make_shared<v3d::render::realtime::vulkan::pipeline::DescriptorPool>(context_->device(),
        context_->ring(), std::vector<VkDescriptorSetLayoutBinding>{block}, 1, "voxel scene");
}

/**
 **/
void Renderer::createUniforms() {
    SceneUniforms uniforms{};
    // the sun, in world space and well above a 256 block wide world
    uniforms.lightPosition = glm::vec4(128.0f, 200.0f, 128.0f, 1.0f);
    uniforms.ambient = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);
    uniforms.diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    uniforms.specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    for (unsigned int i = 0; i < materialCount; i++) {
        // a block reflects its own colour ambiently and diffusely and white specularly -
        // what varies between block types is the colour, not the finish
        uniforms.materials[i].ambient = glm::vec4(palette[i], 1.0f);
        uniforms.materials[i].diffuse = glm::vec4(palette[i], 1.0f);
        uniforms.materials[i].specular = glm::vec4(0.8f, 0.8f, 0.8f, 100.0f);
    }

    uniforms_ = boost::make_shared<v3d::render::realtime::vulkan::memory::DeviceBuffer>(
        context_->device(), context_->uploader(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &uniforms, sizeof(uniforms));

    VkDescriptorSet set = scenePool_->allocate();

    VkDescriptorBufferInfo buffer{};
    buffer.buffer = uniforms_->handle();
    buffer.offset = 0;
    buffer.range = sizeof(uniforms);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &buffer;
    vkUpdateDescriptorSets(context_->device()->handle(), 1, &write, 0, nullptr);

    // a material here is a descriptor set and no texture, which is all the recorder binds
    v3d::render::realtime::vulkan::pipeline::Material material;
    material.set = set;
    material_ = context_->resources()->add(material);
}

/**
 **/
void Renderer::createPipeline() {
    v3d::render::realtime::vulkan::pipeline::Builder builder(context_->device());
    builder.name("voxel-terrain")
        .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
        .shader(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, sizeof(fragmentShader))
        .vertexBinding(0, sizeof(ChunkVertex))
        .vertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ChunkVertex, position))
        .vertexAttribute(1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ChunkVertex, info))
        // a block face is wound counter clockwise seen from outside the block, and the
        // camera's projection flips y - so what reaches the rasterizer is clockwise
        .cull(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)
        .depth(true, true)
        // terrain is opaque, and blending it would cost bandwidth on every fragment of it
        .blend(false)
        .set(context_->frameUniforms()->layout())
        .set(scenePool_->layout())
        .push(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::vec4))
        .colourFormat(context_->colourFormat())
        .depthFormat(context_->depthFormat());

    pipeline_ = context_->resources()->add(builder.build(context_->pipelineCache()));
}

/**
 **/
void Renderer::ui(const boost::shared_ptr<v3d::ui::Engine>& engine) {
    ui_ = engine;
}

/**
 **/
void Renderer::drawTerrain(v3d::render::realtime::Pass* pass) {
    const glm::vec3 eye = scene_->camera()->position();
    const v3d::type::geometry::Frustum frustum(scene_->camera()->projection() * scene_->camera()->view());
    const ChunkMeshPool::EntryMap& entries = meshes_->entries();

    drawnChunks_ = 0;
    meshedChunks_ = 0;
    for (ChunkMeshPool::EntryMap::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        const ChunkMeshPool::Entry& entry = (*it).second;
        if (!entry.mesh) {
            continue;
        }
        meshedChunks_++;
        if (!chunkInView(frustum, entry.origin, entry.size)) {
            continue;
        }
        drawnChunks_++;

        v3d::render::realtime::DrawItem item;
        entry.mesh->describe(&item);
        item.pipeline = pipeline_;
        item.material = material_;

        const glm::vec4 origin(entry.origin, 0.0f);
        std::memcpy(item.push.data(), &origin, sizeof(origin));
        item.pushSize = sizeof(origin);

        item.key.pipeline = static_cast<uint16_t>(pipeline_.id());
        item.key.material = static_cast<uint16_t>(material_.id());
        // near chunks first, so the depth test rejects what is behind them before it is
        // shaded. The far plane is 1000 blocks and a chunk is 16, so quantizing the distance
        // to whole blocks is finer than the ordering can use
        const float distance = glm::length(entry.origin - eye);
        item.key.depth = static_cast<uint16_t>(distance < 65535.0f ? distance : 65535.0f);

        pass->submit(item);
    }
}

/**
 **/
void Renderer::drawDebug(const v3d::ui::shell::StatisticsOverlay::Sample& statistics,
    const v3d::ui::Immediate::Input& tools) {
    const glm::vec3 position = scene_->player()->position();

    v3d::ui::Immediate* layer = screen_->immediate();
    layer->begin(&screen_->canvas(), tools);
    // a line more for each span of the frame that was timed
    const float tall = 154.0f + 26.0f * static_cast<float>(statistics.spans.size());
    if (layer->window(debugTitle, glm::vec2(20.0f, 20.0f), glm::vec2(260.0f, tall), 0.85f)) {
        layer->text(std::string("Voxel ") + VOXEL_VERSION);
        // the loop already keeps a rolling mean, so nothing here averages anything
        layer->text(std::to_string(statistics.mean / 1000000U) + " ms");
        std::stringstream where;
        where.precision(1);
        where << std::fixed << "x " << position.x << "  y " << position.y << "  z " << position.z;
        layer->text(where.str());
        layer->text("chunks " + std::to_string(drawnChunks_) + " / " + std::to_string(meshedChunks_));
        for (const v3d::ui::shell::StatisticsOverlay::Sample::Span& span : statistics.spans) {
            layer->text(v3d::ui::shell::StatisticsOverlay::line(span));
        }
    }
    layer->endWindow();
    layer->end();
}

/**
 **/
void Renderer::draw(const v3d::ui::shell::StatisticsOverlay::Sample& statistics,
    const v3d::ui::Immediate::Input& tools) {
    if (!screen_->begin()) {
        return;
    }
    v3d::render::realtime::Canvas& canvas = screen_->canvas();
    if (screen_->resized()) {
        resize(static_cast<int>(canvas.width()), static_cast<int>(canvas.height()));
    }

    boost::shared_ptr<v3d::render::realtime::Pass> terrain = engine_.frame()->pass(terrainPass);
    terrain->depth(true);
    // one item per chunk through one pipeline and one material, so what the sort does here
    // is order them front to back for the depth test
    terrain->sort(true);
    terrain->camera(scene_->camera()->view(), scene_->camera()->projection());
    drawTerrain(terrain.get());
    scene_->camera()->dirty(false);

    if (debug_) {
        drawDebug(statistics, tools);
    }
    screen_->draw(ui_.get(), statistics);

    // a second pass rather than more items in the first: the terrain is depth tested and
    // sorted front to back, and the text over it is painter ordered and must not be
    boost::shared_ptr<v3d::render::realtime::Pass> overlay = engine_.frame()->pass(overlayPass);
    overlay->keepColour();
    overlay->depth(false);
    engine_.quads()->submit(canvas, overlay.get());

    engine_.renderFrame();
}

/**
 **/
const std::vector<v3d::render::realtime::vulkan::frame::Timings::Timing>& Renderer::timings() const {
    return engine_.timings();
}

/**
 **/
void Renderer::tick(unsigned int /* delta */) {
    // the chunk build has a budget per tick rather than a duration, so how long the last
    // frame took is nothing to it - the loop keeps that, and the debug readout asks
    builder_->build(meshes_, chunkUpdatesPerTick);
}

/**
 **/
void Renderer::resize(int width, int height) {
    const float w = static_cast<float>(width);
    const float h = static_cast<float>(height);

    scene_->camera()->perspective(
        90.0f,  // x fov
        w / h,  // aspect
        0.1f,  // near
        1000.0f);  // far
}

/**
 **/
void Renderer::debug(bool status) {
    debug_ = status;
}

/**
 **/
void Renderer::shutdown() {
    engine_.shutdown();
}

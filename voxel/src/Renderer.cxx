/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <api/asset/Manager.h>
#include <api/log/Logger.h>
#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/Engine3D.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/frame/Timings.h>
#include <api/render/realtime/vulkan/memory/DeviceBuffer.h>
#include <api/render/realtime/vulkan/pipeline/Builder.h>
#include <api/render/realtime/vulkan/pipeline/DescriptorPool.h>
#include <api/render/realtime/vulkan/pipeline/Material.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>
#include <api/type/geometry/Frustum.h>
#include <api/ui/Engine.h>
#include <api/ui/Immediate.h>
#include <api/ui/shell/Screen.h>
#include <api/ui/shell/StatisticsOverlay.h>
#include <api/window/Window.h>
#include <voxel/src/engine/Camera.h>
#include <voxel/src/engine/ChunkMeshBuilder.h>
#include <voxel/src/engine/ChunkVertex.h>
#include <voxel/src/engine/Nanoseconds.h>
#include <voxel/src/engine/SceneUniforms.h>
#include <voxel/src/game/Player.h>
#include <voxel/src/voxel/ChunkCulling.h>
#include <voxel/src/voxel/ChunkMeshPool.h>
#include <voxel/src/voxel/MeshBuilder.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <ios>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Scene.h"
#include "Version.h"

#include <boost/make_shared.hpp>
#include <glm/vec3.hpp>

namespace {

/**
 * The terrain pipeline's shader modules. v3d_add_shader compiles each to SPIR-V at build time
 * with glslc -mfmt=c, which writes it as a C initialiser list that is included here.
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
 * What the debug readout's window is titled, which is also its id in the immediate layer.
 **/
const char* const debugTitle = "Debug";

/**
 * The size the ui and the debug overlay are drawn at. Glyphs are distance fields, so the
 * atlas is scaled to this size rather than rasterized at it.
 **/
const float fontSize = 18.0F;

constexpr glm::vec4 sky(0.4F, 0.6F, 0.9F, 1.0F);
constexpr glm::vec4 textColour(0.95F, 0.95F, 0.95F, 1.0F);
constexpr glm::vec4 white(1.0F, 1.0F, 1.0F, 1.0F);

/**
 * How many chunks are remeshed in one tick. Each mesh waits for its staging copy, so meshing
 * the whole world in one frame would stall that frame visibly.
 **/
const size_t chunkUpdatesPerTick = 16;

/**
 * The sun, in world space and well above a 256 block wide world.
 **/
constexpr glm::vec4 sunPosition(128.0F, 200.0F, 128.0F, 1.0F);
constexpr glm::vec4 ambientLight(0.4F, 0.4F, 0.4F, 1.0F);

/**
 * The specular colour every block shares, with its shininess in the fourth component.
 **/
constexpr glm::vec4 blockSpecular(0.8F, 0.8F, 0.8F, 100.0F);

/**
 * The farthest distance, in blocks, that a draw's depth key can hold.
 **/
constexpr float maxDepthKey = static_cast<float>(std::numeric_limits<uint16_t>::max());

/**
 * Where the debug readout sits and how big it is, in pixels, and the alpha its background is
 * drawn at. Its height is debugHeight plus one debugLineHeight for each timed span.
 **/
constexpr glm::vec2 debugPosition(20.0F, 20.0F);
constexpr float debugWidth = 260.0F;
constexpr float debugHeight = 154.0F;
constexpr float debugLineHeight = 26.0F;
constexpr float debugAlpha = 0.85F;

/**
 * The block palette, indexed by Voxel::BlockType less one. Air is never meshed, so the
 * table starts at dirt.
 **/
constexpr glm::vec3 palette[materialCount] = {
    glm::vec3(0.9F, 0.5F, 0.3F),     // dirt
    glm::vec3(0.13F, 0.56F, 0.19F),  // grass
    glm::vec3(0.9F, 0.88F, 0.58F),   // sand
    glm::vec3(0.75F, 0.75F, 0.75F),  // stone
    glm::vec3(0.52F, 0.52F, 0.52F),  // gravel
    glm::vec3(0.25F, 0.39F, 0.96F),  // water
    glm::vec3(0.16F, 0.16F, 0.16F),  // ore
    glm::vec3(0.5F, 0.29F, 0.02F),   // wood
    glm::vec3(1.0F, 0.47F, 0.12F),   // lava
    glm::vec3(0.76F, 0.87F, 1.0F),   // glass
    glm::vec3(0.29F, 0.29F, 0.29F),  // bedrock
    glm::vec3(0.86F, 0.86F, 0.86F),  // clay
    glm::vec3(0.96F, 0.96F, 0.96F),  // ice
    glm::vec3(0.85F, 0.81F, 0.56F),  // sandstone
    glm::vec3(0.0F, 0.0F, 0.0F),     // obsidian
    glm::vec3(0.91F, 0.91F, 0.91F)   // snow
};

};  // namespace

/**
 **/
Renderer::Renderer(const boost::shared_ptr<Scene> & scene, const boost::shared_ptr<v3d::window::Window>& window,
    const boost::shared_ptr<v3d::log::Logger> & logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager) :
    scene_(scene),
    logger_(logger),
    engine_(logger, assetManager),
    drawnChunks_(0),
    meshedChunks_(0),
    debug_(false) {
    engine_.initialize(window);
    engine_.clearColour(sky);

    context_ = engine_.context();
    if (!context_) {
        throw std::runtime_error("The voxel renderer needs a device to build its pipeline against, and the render engine did not start");
    }

    createLayout();
    createUniforms();
    createPipeline();
    meshes_ = boost::make_shared<ChunkMeshPool>([ring = context_->ring()](std::function<void()> destroy) {
        ring->retire(std::move(destroy));
    });
    builder_ = boost::make_shared<MeshBuilder>(scene_->chunks(),
        ChunkMeshBuilder(context_->device(), context_->uploader()));

    v3d::ui::shell::Screen::Options options;
    options.size = fontSize;
    // the frame time is in the debug readout, so there is no overlay to draw it
    options.statistics = false;
    // the debug readout is an immediate mode panel: it is written as calls every frame
    // rather than kept as a tree in step with what it shows
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
    uniforms.lightPosition = sunPosition;
    uniforms.ambient = ambientLight;
    uniforms.diffuse = glm::vec4(1.0F, 1.0F, 1.0F, 1.0F);
    uniforms.specular = glm::vec4(1.0F, 1.0F, 1.0F, 1.0F);
    for (unsigned int i = 0; i < materialCount; i++) {
        // a block reflects its own colour ambiently and diffusely, and white specularly.
        // Block types differ in colour, not in finish
        uniforms.materials[i].ambient = glm::vec4(palette[i], 1.0F);
        uniforms.materials[i].diffuse = glm::vec4(palette[i], 1.0F);
        uniforms.materials[i].specular = blockSpecular;
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
        .vertexBinding({.binding = 0, .stride = sizeof(ChunkVertex)})
        .vertexAttribute({.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(ChunkVertex, position)})
        .vertexAttribute({.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(ChunkVertex, info)})
        // a block face is wound counter clockwise seen from outside the block. The camera's
        // projection flips y, so what reaches the rasterizer is clockwise
        .cull(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)
        .depth(true, true)
        // terrain is opaque, and blending it would cost bandwidth on every fragment of it
        .blend(false)
        .set(context_->frameUniforms()->layout())
        .set(scenePool_->layout())
        .push({.stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .size = sizeof(glm::vec4)})
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

        const glm::vec4 origin(entry.origin, 0.0F);
        std::memcpy(item.push.data(), &origin, sizeof(origin));
        item.pushSize = sizeof(origin);

        // sort near chunks first, so the depth test rejects what is behind them before it is
        // shaded. The key holds whole blocks, which is fine enough to order chunks 16 blocks
        // wide
        const float distance = glm::length(entry.origin - eye);
        item.key.depth = static_cast<uint16_t>(distance < maxDepthKey ? distance : maxDepthKey);

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
    const float tall = debugHeight + (debugLineHeight * static_cast<float>(statistics.spans.size()));
    if (layer->window(debugTitle, {.position = debugPosition, .size = glm::vec2(debugWidth, tall)}, debugAlpha)) {
        layer->text(std::string("Voxel ") + VOXEL_VERSION);
        // statistics.mean is already the loop's rolling mean of the frame time
        layer->text(std::to_string(statistics.mean / NANOSECONDS_PER_MILLISECOND) + " ms");
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
    const v3d::render::realtime::canvas::Canvas& canvas = screen_->canvas();
    if (screen_->resized()) {
        resize(glm::ivec2(static_cast<int>(canvas.width()), static_cast<int>(canvas.height())));
    }

    const boost::shared_ptr<v3d::render::realtime::Pass> terrain = engine_.frame()->pass(terrainPass);
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

    // the text goes in a second pass. The terrain pass is depth tested and sorted front to
    // back, and the text over it must be drawn in painter order with no depth test
    const boost::shared_ptr<v3d::render::realtime::Pass> overlay = engine_.frame()->pass(overlayPass);
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
    // the chunk build's budget is a number of chunks, not a duration, so delta is not used
    builder_->build(meshes_, chunkUpdatesPerTick);
}

/**
 **/
void Renderer::resize(const glm::ivec2& size) {
    const int width = size.x;
    const int height = size.y;
    const float floatWidth = static_cast<float>(width);
    const float floatHeight = static_cast<float>(height);

    scene_->camera()->perspective({.fovx = Camera::DEFAULT_FOVX, .aspect = floatWidth / floatHeight, .znear = Camera::DEFAULT_ZNEAR, .zfar = Camera::DEFAULT_ZFAR});
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

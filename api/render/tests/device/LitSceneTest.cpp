/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/media/Loaders.h>
#include <api/ecs/component/Emitter.h>
#include <api/ecs/component/Transform.h>
#include <api/image/Compare.h>
#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/DepthOrder.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Grade.h>
#include <api/render/realtime/LitSettings.h>
#include <api/render/realtime/MeshRegistry.h>
#include <api/render/realtime/Particles.h>
#include <api/render/realtime/Meshes.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/SceneUniforms.h>
#include <api/render/realtime/Shadow.h>
#include <api/render/realtime/WorldCanvas.h>
#include <api/render/realtime/component/Mesh.h>
#include <api/render/realtime/component/Particles.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/DepthBuffer.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/renderer/Lit.h>
#include <api/render/realtime/vulkan/renderer/World.h>
#include <api/type/Model.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Isometric.h>
#include <api/type/effect/Weather.h>

#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <entt/entt.hpp>

#include "Headless.h"

using v3d::ecs::component::Transform;
using v3d::render::realtime::Frame;
using v3d::render::realtime::LitSettings;
using v3d::render::realtime::MeshHandle;
using v3d::render::realtime::MeshRegistry;
using v3d::render::realtime::Pass;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::vulkan::frame::Recorder;
using v3d::render::realtime::vulkan::frame::RenderTarget;
using v3d::render::realtime::vulkan::renderer::Lit;
using v3d::render::realtime::vulkan::renderer::World;

namespace {

// the lit pipelines compute in linear light, and an SRGB target encodes it on store
const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_SRGB;
const uint32_t width = 128;
const uint32_t height = 128;

/**
 * A unit cube centred on the origin, a face at a time so that each face has its own normal,
 * wound counter clockwise seen from outside as a glTF model is.
 **/
v3d::type::Model cube(const glm::vec4& colour) {
    v3d::type::Model model;
    const glm::vec3 normals[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (const glm::vec3& n : normals) {
        // two axes across the face, chosen so that u x v points along the normal
        const glm::vec3 u = glm::abs(n.y) > 0.5f ? glm::vec3(n.y, 0, 0) * -1.0f : glm::vec3(-n.z, 0, n.x);
        const glm::vec3 v = glm::cross(n, u);
        const uint32_t first = static_cast<uint32_t>(model.vertices().size());
        const glm::vec3 corners[4] = {
            n * 0.5f - u * 0.5f - v * 0.5f, n * 0.5f + u * 0.5f - v * 0.5f,
            n * 0.5f + u * 0.5f + v * 0.5f, n * 0.5f - u * 0.5f + v * 0.5f
        };
        for (const glm::vec3& corner : corners) {
            v3d::type::Model::Vertex vertex;
            vertex.position = corner;
            vertex.normal = n;
            model.vertices().push_back(vertex);
        }
        model.indices().insert(model.indices().end(), {first, first + 1, first + 2, first, first + 2, first + 3});
    }
    v3d::type::Model::Material material;
    material.baseColour = colour;
    model.materials().push_back(material);
    model.parts().push_back({0, static_cast<uint32_t>(model.indices().size()), 0});
    return model;
}

/**
 * Two cubes side by side along x in one model, a part each: red on the left, green on the
 * right.
 **/
v3d::type::Model twoCubes() {
    v3d::type::Model model = cube(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    for (v3d::type::Model::Vertex& vertex : model.vertices()) {
        vertex.position.x -= 0.75f;
    }
    const v3d::type::Model green = cube(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    const uint32_t base = static_cast<uint32_t>(model.vertices().size());
    const uint32_t first = static_cast<uint32_t>(model.indices().size());
    for (v3d::type::Model::Vertex vertex : green.vertices()) {
        vertex.position.x += 0.75f;
        model.vertices().push_back(vertex);
    }
    for (const uint32_t index : green.indices()) {
        model.indices().push_back(base + index);
    }
    model.materials().push_back(green.materials()[0]);
    model.parts().push_back({first, static_cast<uint32_t>(green.indices().size()), 1});
    return model;
}

entt::entity place(entt::registry* registry, const MeshHandle& mesh, const glm::vec3& position, const glm::vec3& scale,
    bool castsShadow) {
    const entt::entity entity = registry->create();
    Transform transform;
    transform.position = position;
    transform.scale = scale;
    registry->emplace<Transform>(entity, transform);
    registry->emplace<v3d::render::realtime::component::Mesh>(entity, v3d::render::realtime::component::Mesh{mesh, castsShadow});
    return entity;
}

/**
 * Capture a target's colour into a picture and read it back, so a case can look at its texels.
 **/
boost::shared_ptr<v3d::image::Image> readBack(v3d::test::Headless* headless, VkCommandBuffer commands,
    const boost::shared_ptr<RenderTarget>& target, const std::string& path) {
    Capture capture(headless->device, headless->logger);
    Capture::Source source;
    source.image = target->image();
    source.extent = target->extent();
    source.format = target->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture.record(commands, source);
    headless->submitAndWait(commands);
    if (!capture.write(path)) {
        return boost::shared_ptr<v3d::image::Image>();
    }
    v3d::image::reader::Png png(headless->logger);
    return png.read(path);
}

/**
 * How the shadow changed a picture: the pixels that are white ground without it and grey with
 * it, and the pixels that changed in any other way. A white surface in shadow drops from the lit
 * band to the mid one, and its edge blends the two, so it stays grey and gets no lighter.
 **/
struct Shading final {
    std::size_t darkened = 0;
    std::size_t otherwise = 0;
};

Shading compare(const boost::shared_ptr<v3d::image::Image>& without, const boost::shared_ptr<v3d::image::Image>& with) {
    Shading shading;
    for (std::size_t texel = 0; texel < static_cast<std::size_t>(with->width()) * with->height(); texel++) {
        const unsigned char* before = without->data() + texel * 4;
        const unsigned char* after = with->data() + texel * 4;
        if (before[0] == after[0] && before[1] == after[1] && before[2] == after[2]) {
            continue;
        }
        const bool white = before[0] == 255 && before[1] == 255 && before[2] == 255;
        const bool grey = after[0] == after[1] && after[1] == after[2] && after[0] < 255;
        if (white && grey) {
            shading.darkened++;
        } else {
            shading.otherwise++;
        }
    }
    return shading;
}

/**
 * A white ground and a red cube standing on it, drawn after a shadow pass, and the picture.
 *
 * @param strength LitSettings::shadowStrength, so a case can draw the same frame with the
 *        shadow ignored
 * @param frames how many frames to draw into the same shadow map. Every frame but the last is
 *        submitted without waiting, as a presented frame is, so two of them are in flight at once
 **/
boost::shared_ptr<v3d::image::Image> drawShadowed(v3d::test::Headless* headless, float strength, const std::string& path,
    int frames = 1) {
    const uint32_t mapSize = 256;
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless->device, headless->context->ring(),
        width, height, colourFormat, true);
    boost::shared_ptr<RenderTarget> map = boost::make_shared<RenderTarget>(headless->device, headless->context->ring(),
        mapSize, mapSize, VK_FORMAT_UNDEFINED, true, true);

    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless->logger);

    v3d::asset::media::registerLoaders(*assets, headless->logger);
    MeshRegistry meshes(headless->logger, headless->context, assets);
    const MeshHandle crate = meshes.add("crate", cube(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)));
    const MeshHandle ground = meshes.add("ground", cube(glm::vec4(1.0f)));

    Lit lit(headless->device, headless->context->pipelineCache(), headless->context->resources(), headless->context->ring(),
        headless->context->frameUniforms(), headless->context->textures(), colourFormat, target->depthFormat(), map->depthFormat());

    entt::registry registry;
    place(&registry, crate, glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f), true);
    // thin, and topped at zero, so the cube stands on it. It casts nothing, so the fit is the
    // cube's alone
    place(&registry, ground, glm::vec3(0.0f, -0.05f, 0.0f), glm::vec3(6.0f, 0.1f, 6.0f), false);

    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(0.0f));
    orbit.zoom(3.0f);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(0.1f, 100.0f);
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();

    LitSettings settings;
    settings.outline = 0.0f;
    settings.shadowStrength = strength;

    const std::optional<v3d::render::realtime::shadow::Bounds> bounds =
        v3d::render::realtime::shadow::fit(registry, {}, 2.0f);
    if (!bounds) {
        BOOST_ERROR("a cube that casts gave no fit");
        return boost::shared_ptr<v3d::image::Image>();
    }
    const glm::mat4 light = v3d::render::realtime::shadow::light(settings.light, bounds->centre, bounds->radius);
    VkDescriptorSet scene = lit.scene(v3d::render::realtime::pack(settings, light, 1.0f / mapSize),
        headless->context->textures()->depthTexture(*map));

    for (int drawn = 1; ; drawn++) {
        // the lit pass is made first, as an engine's colour pass is, and reads the map, so the
        // frame records the shadow pass ahead of it
        Frame frame;
        boost::shared_ptr<Pass> pass = frame.pass("lit");
        boost::shared_ptr<Pass> casting = frame.pass("shadow");
        casting->target(map);
        casting->depth(true);
        casting->scene(scene);
        casting->depthBias(settings.constantBias, settings.slopeBias);
        v3d::render::realtime::casters(registry, 1.0f, meshes, lit, casting.get());

        pass->reads(map);
        pass->target(target);
        pass->depth(true);
        pass->clearColour(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
        pass->camera(camera.view(), camera.projection());
        pass->scene(scene);
        v3d::render::realtime::meshes(registry, 1.0f, meshes, lit, settings.outline, pass.get());

        VkCommandBuffer commands = headless->context->ring()->begin();
        Recorder::record(commands, frame, Recorder::Target(), *headless->context->resources(),
            headless->context->frameUniforms().get());
        if (drawn == frames) {
            return readBack(headless, commands, target, path);
        }
        headless->submit(commands);
    }
}

};  // namespace

BOOST_AUTO_TEST_SUITE(lit_scene_test)

/**
 * A lit cube, outlined, drawn through the recorder from an entity. Lighting arithmetic is left
 * to the implementation, so there is no reference picture. The case checks that the validation
 * layer reports no errors, and checks one pixel. The pixel at the centre lands on the cube's
 * top face, which faces the key light and so is in the lit band of the cube's own colour. The
 * same pixel checks the winding: with the faces the wrong way round, the near side of the
 * outline hull would cover it in black.
 *
 * The picture is always written to data_out/lit_cube.png for a person to look at.
 **/
BOOST_AUTO_TEST_CASE(a_lit_entity_is_drawn_and_silent) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, colourFormat, true);

    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless.logger);

    v3d::asset::media::registerLoaders(*assets, headless.logger);
    MeshRegistry meshes(headless.logger, headless.context, assets);
    const MeshHandle crate = meshes.add("crate", cube(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)));

    Lit lit(headless.device, headless.context->pipelineCache(), headless.context->resources(), headless.context->ring(),
        headless.context->frameUniforms(), headless.context->textures(), colourFormat, target->depthFormat(), VK_FORMAT_UNDEFINED);

    entt::registry registry;
    const entt::entity entity = registry.create();
    registry.emplace<Transform>(entity);
    registry.emplace<v3d::render::realtime::component::Mesh>(entity, v3d::render::realtime::component::Mesh{crate, true});

    // the tree's own orthographic camera, which builds Vulkan clip space and so decides which
    // way round a face is on screen. Steeper than its default 45 degrees, so that the ray
    // through the centre of the picture meets the top face rather than an edge
    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(0.0f));
    orbit.zoom(1.5f);
    orbit.elevation(1.0471976f);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(0.1f, 100.0f);
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();

    // thicker than the default, which at this zoom is under a pixel and can cover no pixel
    // centre at all
    LitSettings settings;
    settings.outline = 0.1f;

    // the frame is built before it is begun, as an engine builds one during its tick. A scene
    // waits for its slot to be free, and a begun frame's fence is not signalled until it is
    // submitted
    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("lit");
    pass->target(target);
    pass->depth(true);
    pass->clearColour(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    pass->camera(camera.view(), camera.projection());
    pass->scene(lit.scene(v3d::render::realtime::pack(settings, glm::mat4(1.0f), 0.0f)));
    v3d::render::realtime::meshes(registry, 1.0f, meshes, lit, settings.outline, pass.get());

    VkCommandBuffer commands = headless.context->ring()->begin();
    // the pass names its own target, so the frame is given no image
    Recorder::record(commands, frame, Recorder::Target(), *headless.context->resources(),
        headless.context->frameUniforms().get());

    Capture capture(headless.device, headless.logger);
    Capture::Source source;
    source.image = target->image();
    source.extent = target->extent();
    source.format = target->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture.record(commands, source);
    headless.submitAndWait(commands);

    BOOST_CHECK(headless.silent());
    BOOST_REQUIRE(capture.write("data_out/lit_cube.png"));

    v3d::image::reader::Png png(headless.logger);
    boost::shared_ptr<v3d::image::Image> picture = png.read("data_out/lit_cube.png");
    BOOST_REQUIRE(picture);
    const unsigned char* centre = picture->data() + (static_cast<std::size_t>(height / 2) * width + width / 2) * 4;
    BOOST_TEST_MESSAGE("centre " << int(centre[0]) << "," << int(centre[1]) << "," << int(centre[2]));
    BOOST_CHECK_GT(centre[0], 200);
    BOOST_CHECK_LT(centre[1], 10);
    BOOST_CHECK_LT(centre[2], 10);

    // the outline is black and nothing else in the picture is, so a black pixel is one
    std::size_t outlined = 0;
    for (std::size_t texel = 0; texel < static_cast<std::size_t>(width) * height; texel++) {
        const unsigned char* at = picture->data() + texel * 4;
        if (at[0] == 0 && at[1] == 0 && at[2] == 0) {
            outlined++;
        }
    }
    BOOST_TEST_MESSAGE("outline pixels " << outlined);
    BOOST_CHECK_GT(outlined, 0U);
}

/**
 * An entity whose mesh was released is skipped rather than drawn from a stale entry, so
 * meshes() submits nothing for it and nothing for its outline.
 **/
BOOST_AUTO_TEST_CASE(a_released_mesh_is_not_walked) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless.logger);
    v3d::asset::media::registerLoaders(*assets, headless.logger);
    MeshRegistry meshes(headless.logger, headless.context, assets);
    Lit lit(headless.device, headless.context->pipelineCache(), headless.context->resources(), headless.context->ring(),
        headless.context->frameUniforms(), headless.context->textures(), colourFormat,
        v3d::render::realtime::vulkan::frame::DepthBuffer::chooseFormat(headless.device->physical()), VK_FORMAT_UNDEFINED);

    entt::registry registry;
    const MeshHandle kept = meshes.add("kept", cube(glm::vec4(1.0f)));
    const MeshHandle released = meshes.add("released", cube(glm::vec4(1.0f)));
    for (const MeshHandle& handle : {kept, released}) {
        const entt::entity entity = registry.create();
        registry.emplace<Transform>(entity);
        registry.emplace<v3d::render::realtime::component::Mesh>(entity, v3d::render::realtime::component::Mesh{handle, true});
    }
    BOOST_REQUIRE(meshes.release(released));

    Pass pass("lit");
    v3d::render::realtime::meshes(registry, 1.0f, meshes, lit, 0.1f, &pass);

    // an outline and a surface for the one that is left
    BOOST_CHECK_EQUAL(pass.items().size(), 2U);
}

/**
 * A caster is drawn into a shadow map at the depth the light's matrix gives it. This is the
 * depth target's case again, with the quads standing in the world and drawn through Lit's
 * shadow pipeline and shadow::light. The depths are exact for the same reason: each front
 * face is a plane of one depth, at a quarter and three quarters of the light's range.
 *
 * A third entity that casts no shadow stands nearer the light over the right quad. If it were
 * drawn into the map, it would put its own depth there.
 **/
BOOST_AUTO_TEST_CASE(a_caster_is_drawn_into_the_shadow_map_at_its_depth) {
    const uint32_t size = 16;
    v3d::test::Headless headless(colourFormat, size, size);
    boost::shared_ptr<RenderTarget> map = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        size, size, VK_FORMAT_UNDEFINED, true, true);
    if (map->depthFormat() != VK_FORMAT_D32_SFLOAT) {
        BOOST_TEST_MESSAGE("The device gives no sampled D32_SFLOAT, so there is no exact depth to compare");
        return;
    }

    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless.logger);

    v3d::asset::media::registerLoaders(*assets, headless.logger);
    MeshRegistry meshes(headless.logger, headless.context, assets);
    const MeshHandle block = meshes.add("block", cube(glm::vec4(1.0f)));
    Lit lit(headless.device, headless.context->pipelineCache(), headless.context->resources(), headless.context->ring(),
        headless.context->frameUniforms(), headless.context->textures(), colourFormat,
        v3d::render::realtime::vulkan::frame::DepthBuffer::chooseFormat(headless.device->physical()), map->depthFormat());

    // the light looks along +z from two units out, over a sphere of one. A face at z = -1 is
    // therefore a quarter of the way into its range, and one at z = 1 three quarters. Each
    // block is half a unit wide and spans the same rectangles as the depth target's two quads
    entt::registry registry;
    const glm::vec3 scale(0.5f, 1.0f, 1.0f);
    place(&registry, block, glm::vec3(-0.5f, 0.0f, -0.5f), scale, true);
    place(&registry, block, glm::vec3(0.5f, 0.0f, 1.5f), scale, true);
    place(&registry, block, glm::vec3(0.5f, 0.0f, 0.5f), scale, false);

    const glm::mat4 light = v3d::render::realtime::shadow::light(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f), 1.0f);

    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("shadow");
    pass->target(map);
    pass->depth(true);
    pass->scene(lit.scene(v3d::render::realtime::pack(LitSettings(), light, 1.0f / size)));
    // the pipeline is biased, so the pass names a bias - none, so that the depths stay exact
    pass->depthBias(0.0f, 0.0f);
    v3d::render::realtime::casters(registry, 1.0f, meshes, lit, pass.get());
    BOOST_CHECK_EQUAL(pass->items().size(), 2U);

    VkCommandBuffer commands = headless.context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless.context->resources(),
        headless.context->frameUniforms().get());

    Capture capture(headless.device, headless.logger);
    Capture::Source source;
    source.image = map->depthImage();
    source.extent = map->extent();
    source.format = map->depthFormat();
    source.layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
    source.depth = true;
    capture.record(commands, source);
    headless.submitAndWait(commands);

    BOOST_CHECK(headless.silent());
    const std::vector<float> depths = capture.depth();
    BOOST_REQUIRE_EQUAL(depths.size(), static_cast<std::size_t>(size) * size);
    const auto at = [&depths, size](uint32_t x, uint32_t y) { return depths[static_cast<std::size_t>(y) * size + x]; };
    BOOST_CHECK_EQUAL(at(4, 8), 0.25f);
    BOOST_CHECK_EQUAL(at(11, 8), 0.75f);
    BOOST_CHECK_EQUAL(at(8, 8), 1.0f);
    BOOST_CHECK_EQUAL(at(0, 0), 1.0f);
}

/**
 * Two frames in flight share one shadow map. The second frame's shadow pass writes the map
 * while the first frame's lit pass may still be sampling it, so the barrier that opens the map
 * has to wait for that read. Only synchronization validation reports it when it does not, so
 * the case checks that the layer is silent.
 **/
BOOST_AUTO_TEST_CASE(a_shadow_map_is_shared_by_frames_in_flight) {
    v3d::test::Headless headless(colourFormat, width, height);

    boost::shared_ptr<v3d::image::Image> picture = drawShadowed(&headless, 1.0f, "data_out/lit_shadow_twice.png", 2);
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());
}

/**
 * A cube on the ground, with a shadow pass before the lit pass and the map read through the
 * scene set. Where the shadow falls depends on PCF and the implementation's filtering, so there
 * is no reference picture. The case checks that the validation layer reports no errors, and
 * compares the frame against the same frame drawn with the shadow ignored: some white ground
 * went grey, and nothing else changed. Too little bias makes a surface shadow itself, which
 * would show as a change on the cube.
 *
 * The picture is written to data_out/lit_shadow.png for a person to look at.
 **/
BOOST_AUTO_TEST_CASE(a_shadow_falls_on_the_ground) {
    v3d::test::Headless headless(colourFormat, width, height);

    boost::shared_ptr<v3d::image::Image> ignored = drawShadowed(&headless, 0.0f, "data_out/lit_unshadowed.png");
    BOOST_REQUIRE(ignored);
    boost::shared_ptr<v3d::image::Image> picture = drawShadowed(&headless, 1.0f, "data_out/lit_shadow.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());

    const Shading shading = compare(ignored, picture);
    BOOST_TEST_MESSAGE("ground darkened " << shading.darkened << ", anything else changed " << shading.otherwise);
    BOOST_CHECK_GT(shading.darkened, 0U);
    BOOST_CHECK_EQUAL(shading.otherwise, 0U);
}

/**
 * A full-size lit game scene built from this tree's fixtures. It holds a ground that casts
 * nothing, two upright figures and ten props at set yaws, under one key light at LitSettings'
 * defaults. It is seen through an orthographic camera at 45 degrees at 1280 by 720. The frame
 * is a shadow pass, the lit scene into an sRGB target, and a grade into a target in an sRGB
 * swapchain format. The grade uses the identity table.
 *
 * The passes are created in the opposite order, grade first, as an engine creates its colour
 * pass before a game adds anything. Each pass names what it reads, so the frame must record a
 * pass that draws into a target before any pass that reads it.
 *
 * Lighting, PCF and filtering are left to the implementation, so there is no reference picture.
 * The case checks that the validation layer reports no errors, here and on lavapipe in CI, and
 * that the frame drew something. data_out/lit_scene.png is for a person to look at.
 **/
BOOST_AUTO_TEST_CASE(full_lit_scene_is_drawn_and_silent) {
    const uint32_t frameWidth = 1280;
    const uint32_t frameHeight = 720;
    const uint32_t mapSize = 2048;
    const VkFormat swapchainFormat = VK_FORMAT_B8G8R8A8_SRGB;

    v3d::test::Headless headless(swapchainFormat, frameWidth, frameHeight);
    boost::shared_ptr<RenderTarget> scene = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        frameWidth, frameHeight, swapchainFormat, true);
    boost::shared_ptr<RenderTarget> map = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        mapSize, mapSize, VK_FORMAT_UNDEFINED, true, true);
    boost::shared_ptr<RenderTarget> output = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        frameWidth, frameHeight, swapchainFormat);

    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless.logger);

    v3d::asset::media::registerLoaders(*assets, headless.logger);
    MeshRegistry meshes(headless.logger, headless.context, assets);
    const MeshHandle ground = meshes.add("ground", cube(glm::vec4(0.55f, 0.6f, 0.5f, 1.0f)));
    const MeshHandle figure = meshes.add("figure", cube(glm::vec4(0.8f, 0.3f, 0.2f, 1.0f)));
    const MeshHandle crate = meshes.add("crate", cube(glm::vec4(0.6f, 0.45f, 0.3f, 1.0f)));
    const MeshHandle primitives = meshes.load("three_primitives.glb");
    const MeshHandle textured = meshes.load("embedded_texture.glb");
    BOOST_REQUIRE(meshes.resolve(primitives) != nullptr);
    BOOST_REQUIRE(meshes.resolve(textured) != nullptr);

    Lit lit(headless.device, headless.context->pipelineCache(), headless.context->resources(), headless.context->ring(),
        headless.context->frameUniforms(), headless.context->textures(), swapchainFormat, scene->depthFormat(), map->depthFormat());
    v3d::render::realtime::Grade grade(headless.logger, headless.context, swapchainFormat, VK_FORMAT_UNDEFINED);
    const v3d::render::realtime::MaterialHandle graded = grade.source(*scene);

    entt::registry registry;
    const auto stand = [&registry](const MeshHandle& mesh, const glm::vec3& position, float yaw, const glm::vec3& scale,
        bool castsShadow) {
        const entt::entity entity = registry.create();
        Transform transform;
        transform.position = position;
        transform.rotation = v3d::ecs::component::aboutY(yaw);
        transform.scale = scale;
        registry.emplace<Transform>(entity, transform);
        registry.emplace<v3d::render::realtime::component::Mesh>(entity, v3d::render::realtime::component::Mesh{mesh, castsShadow});
    };
    stand(ground, glm::vec3(0.0f, -0.05f, 0.0f), 0.0f, glm::vec3(14.0f, 0.1f, 14.0f), false);
    stand(figure, glm::vec3(-1.0f, 0.9f, 0.5f), 0.3f, glm::vec3(0.6f, 1.8f, 0.4f), true);
    stand(figure, glm::vec3(1.2f, 0.9f, -0.4f), -0.8f, glm::vec3(0.6f, 1.8f, 0.4f), true);
    // ten props, in a ring around the two, each turned its own way
    const MeshHandle props[3] = {crate, primitives, textured};
    for (int prop = 0; prop < 10; prop++) {
        const float around = static_cast<float>(prop) * 0.6283185f;
        const glm::vec3 at(std::cos(around) * 4.0f, 0.5f, std::sin(around) * 4.0f);
        stand(props[prop % 3], at, around * 1.7f, glm::vec3(1.0f), true);
    }

    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(0.0f));
    orbit.zoom(6.0f);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(0.1f, 100.0f);
    camera.profile().pixelAspect(static_cast<float>(frameWidth) / static_cast<float>(frameHeight));
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();

    // the default settings, and three metres of margin for a figure's height and its shadow
    const LitSettings settings;
    const std::optional<v3d::render::realtime::shadow::Bounds> bounds = v3d::render::realtime::shadow::fit(registry, {}, 3.0f);
    if (!bounds) {
        BOOST_ERROR("a scene with casters gave no fit");
        return;
    }
    const glm::mat4 light = v3d::render::realtime::shadow::light(settings.light, bounds->centre, bounds->radius);
    VkDescriptorSet sceneSet = lit.scene(v3d::render::realtime::pack(settings, light, 1.0f / static_cast<float>(mapSize)),
        headless.context->textures()->depthTexture(*map));

    Frame frame;
    boost::shared_ptr<Pass> post = frame.pass("grade");
    post->target(output);
    post->reads(scene);
    grade.submit(graded, post.get());

    boost::shared_ptr<Pass> drawn = frame.pass("lit");
    drawn->target(scene);
    drawn->depth(true);
    drawn->reads(map);
    drawn->clearColour(glm::vec4(0.25f, 0.3f, 0.35f, 1.0f));
    drawn->camera(camera.view(), camera.projection());
    drawn->scene(sceneSet);
    v3d::render::realtime::meshes(registry, 1.0f, meshes, lit, settings.outline, drawn.get());

    boost::shared_ptr<Pass> casting = frame.pass("shadow");
    casting->target(map);
    casting->depth(true);
    casting->scene(sceneSet);
    casting->depthBias(settings.constantBias, settings.slopeBias);
    v3d::render::realtime::casters(registry, 1.0f, meshes, lit, casting.get());

    const std::vector<boost::shared_ptr<Pass>> ordered = frame.ordered();
    BOOST_REQUIRE_EQUAL(ordered.size(), 3U);
    BOOST_CHECK_EQUAL(ordered[0]->name(), "shadow");
    BOOST_CHECK_EQUAL(ordered[1]->name(), "lit");
    BOOST_CHECK_EQUAL(ordered[2]->name(), "grade");

    VkCommandBuffer commands = headless.context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless.context->resources(),
        headless.context->frameUniforms().get());
    boost::shared_ptr<v3d::image::Image> picture = readBack(&headless, commands, output, "data_out/lit_scene.png");
    grade.release(graded);
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());

    // something was drawn: the clear colour is not the whole frame, and neither is any one colour
    const unsigned char* first = picture->data();
    std::size_t differing = 0;
    for (std::size_t texel = 0; texel < static_cast<std::size_t>(frameWidth) * frameHeight; texel++) {
        const unsigned char* at = picture->data() + texel * 4;
        if (at[0] != first[0] || at[1] != first[1] || at[2] != first[2]) {
            differing++;
        }
    }
    BOOST_TEST_MESSAGE("pixels unlike the corner " << differing);
    BOOST_CHECK_GT(differing, static_cast<std::size_t>(frameWidth) * frameHeight / 4);
}

/**
 * A model of two parts is drawn a part at a time, each with its own base colour: one model of a
 * red cube and a green one, side by side. The lit band of each colour appears. It would not if
 * meshes() drew the whole model with one part's colour.
 *
 * The picture is written to data_out/lit_parts.png.
 **/
BOOST_AUTO_TEST_CASE(a_model_is_drawn_a_part_at_a_time) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, colourFormat, true);

    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless.logger);

    v3d::asset::media::registerLoaders(*assets, headless.logger);
    MeshRegistry meshes(headless.logger, headless.context, assets);
    const MeshHandle pair = meshes.add("pair", twoCubes());

    Lit lit(headless.device, headless.context->pipelineCache(), headless.context->resources(), headless.context->ring(),
        headless.context->frameUniforms(), headless.context->textures(), colourFormat, target->depthFormat(), VK_FORMAT_UNDEFINED);

    entt::registry registry;
    place(&registry, pair, glm::vec3(0.0f), glm::vec3(1.0f), false);

    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(0.0f));
    orbit.zoom(2.5f);
    orbit.elevation(1.0471976f);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(0.1f, 100.0f);
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();

    LitSettings settings;
    settings.outline = 0.0f;

    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("lit");
    pass->target(target);
    pass->depth(true);
    pass->clearColour(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    pass->camera(camera.view(), camera.projection());
    pass->scene(lit.scene(v3d::render::realtime::pack(settings, glm::mat4(1.0f), 0.0f)));
    v3d::render::realtime::meshes(registry, 1.0f, meshes, lit, settings.outline, pass.get());

    VkCommandBuffer commands = headless.context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless.context->resources(),
        headless.context->frameUniforms().get());
    boost::shared_ptr<v3d::image::Image> picture = readBack(&headless, commands, target, "data_out/lit_parts.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());

    // the lit band of each part's own colour, which is pure in its channel
    std::size_t red = 0;
    std::size_t green = 0;
    for (std::size_t texel = 0; texel < static_cast<std::size_t>(width) * height; texel++) {
        const unsigned char* at = picture->data() + texel * 4;
        if (at[0] > 200 && at[1] < 10 && at[2] < 10) {
            red++;
        }
        if (at[1] > 200 && at[0] < 10 && at[2] < 10) {
            green++;
        }
    }
    BOOST_TEST_MESSAGE("red " << red << ", green " << green);
    BOOST_CHECK_GT(red, 0U);
    BOOST_CHECK_GT(green, 0U);
}

/**
 * World quads drawn in the lit pass after its meshes are depth-tested against them. A green
 * ground quad under a red cube, submitted after the cube, is hidden where the cube stands and
 * seen everywhere else. A world quad drawn without the scene's depth would cover the cube's top
 * face at the centre of the picture. The quads go through a World built against the scene
 * target's own formats, as particles in a lit scene do.
 *
 * The picture is written to data_out/lit_world_quads.png for a person to look at.
 **/
BOOST_AUTO_TEST_CASE(world_quads_are_hidden_by_a_lit_scene) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, colourFormat, true);

    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless.logger);

    v3d::asset::media::registerLoaders(*assets, headless.logger);
    MeshRegistry meshes(headless.logger, headless.context, assets);
    const MeshHandle crate = meshes.add("crate", cube(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)));

    Lit lit(headless.device, headless.context->pipelineCache(), headless.context->resources(), headless.context->ring(),
        headless.context->frameUniforms(), headless.context->textures(), colourFormat, target->depthFormat(), VK_FORMAT_UNDEFINED);
    World world(headless.device, headless.context->pipelineCache(), headless.context->resources(),
        headless.context->ring(), headless.context->frameUniforms(), headless.context->textures(), colourFormat,
        target->depthFormat());

    entt::registry registry;
    place(&registry, crate, glm::vec3(0.0f), glm::vec3(1.0f), false);

    // the first case's camera, whose centre ray meets the cube's top face
    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(0.0f));
    orbit.zoom(1.5f);
    orbit.elevation(1.0471976f);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(0.1f, 100.0f);
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();

    LitSettings settings;
    settings.outline = 0.0f;

    v3d::render::realtime::WorldCanvas ground;
    ground.quad({glm::vec3(-8.0f, -1.0f, -8.0f), glm::vec3(8.0f, -1.0f, -8.0f), glm::vec3(8.0f, -1.0f, 8.0f),
        glm::vec3(-8.0f, -1.0f, 8.0f)}, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));

    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("lit");
    pass->target(target);
    pass->depth(true);
    pass->clearColour(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    pass->camera(camera.view(), camera.projection());
    pass->scene(lit.scene(v3d::render::realtime::pack(settings, glm::mat4(1.0f), 0.0f)));
    v3d::render::realtime::meshes(registry, 1.0f, meshes, lit, settings.outline, pass.get());
    world.submit(ground, pass.get());

    VkCommandBuffer commands = headless.context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless.context->resources(),
        headless.context->frameUniforms().get());
    const boost::shared_ptr<v3d::image::Image> picture = readBack(&headless, commands, target, "data_out/lit_world_quads.png");

    BOOST_CHECK(headless.silent());
    BOOST_REQUIRE(picture);
    const unsigned char* centre = picture->data() + (static_cast<std::size_t>(height / 2) * width + width / 2) * 4;
    BOOST_TEST_MESSAGE("centre " << int(centre[0]) << "," << int(centre[1]) << "," << int(centre[2]));
    BOOST_CHECK_GT(centre[0], 200);
    BOOST_CHECK_LT(centre[1], 10);

    const unsigned char* corner = picture->data() + (static_cast<std::size_t>(4) * width + 4) * 4;
    BOOST_TEST_MESSAGE("corner " << int(corner[0]) << "," << int(corner[1]) << "," << int(corner[2]));
    BOOST_CHECK_LT(corner[0], 10);
    BOOST_CHECK_GT(corner[1], 200);
}

/**
 * The light's colour multiplies the lit band. The top face of a white cube faces the key and so
 * is in the lit band. It is white under a white light and red under a red one. The lit band's
 * multiplier is one and the colours are whole, so both are exact.
 **/
BOOST_AUTO_TEST_CASE(the_light_has_a_colour) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless.logger);
    v3d::asset::media::registerLoaders(*assets, headless.logger);
    MeshRegistry meshes(headless.logger, headless.context, assets);
    const MeshHandle crate = meshes.add("crate", cube(glm::vec4(1.0f)));

    entt::registry registry;
    place(&registry, crate, glm::vec3(0.0f), glm::vec3(1.0f), false);

    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(0.0f));
    orbit.zoom(1.5f);
    orbit.elevation(1.0471976f);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(0.1f, 100.0f);
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();

    for (const glm::vec3& colour : {glm::vec3(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)}) {
        boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
            width, height, colourFormat, true);
        Lit lit(headless.device, headless.context->pipelineCache(), headless.context->resources(), headless.context->ring(),
            headless.context->frameUniforms(), headless.context->textures(), colourFormat, target->depthFormat(), VK_FORMAT_UNDEFINED);

        LitSettings settings;
        settings.outline = 0.0f;
        settings.colour = colour;

        Frame frame;
        boost::shared_ptr<Pass> pass = frame.pass("lit");
        pass->target(target);
        pass->depth(true);
        pass->clearColour(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
        pass->camera(camera.view(), camera.projection());
        pass->scene(lit.scene(v3d::render::realtime::pack(settings, glm::mat4(1.0f), 0.0f)));
        v3d::render::realtime::meshes(registry, 1.0f, meshes, lit, settings.outline, pass.get());

        VkCommandBuffer commands = headless.context->ring()->begin();
        Recorder::record(commands, frame, Recorder::Target(), *headless.context->resources(),
            headless.context->frameUniforms().get());
        const boost::shared_ptr<v3d::image::Image> picture = readBack(&headless, commands, target, "data_out/lit_coloured.png");
        BOOST_REQUIRE(picture);

        const unsigned char* centre = picture->data() + (static_cast<std::size_t>(height / 2) * width + width / 2) * 4;
        BOOST_TEST_MESSAGE("centre " << int(centre[0]) << "," << int(centre[1]) << "," << int(centre[2]));
        BOOST_CHECK_EQUAL(static_cast<int>(centre[0]), 255);
        BOOST_CHECK_EQUAL(static_cast<int>(centre[1]), colour.g > 0.0f ? 255 : 0);
        BOOST_CHECK_EQUAL(static_cast<int>(centre[2]), colour.b > 0.0f ? 255 : 0);
    }
    BOOST_CHECK(headless.silent());
}

/**
 * Rain in a lit scene under a blue light at night. A shower falls over the look of a red cube on
 * a white ground. It is drawn as streaks along each drop's velocity and added to the scene in the
 * lit pass. A streak's appearance depends on filtering and blending, which differ between
 * conformant drivers, so there is no reference picture. The case checks that the validation
 * layer reports no errors and that the rain moves between frames. The frames go to
 * data_out/rain_*.png for a person to look at.
 **/
BOOST_AUTO_TEST_CASE(rain_falls_in_a_lit_scene) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless.logger);
    v3d::asset::media::registerLoaders(*assets, headless.logger);
    MeshRegistry meshes(headless.logger, headless.context, assets);
    const MeshHandle crate = meshes.add("crate", cube(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)));
    const MeshHandle ground = meshes.add("ground", cube(glm::vec4(1.0f)));

    entt::registry registry;
    place(&registry, crate, glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f), false);
    place(&registry, ground, glm::vec3(0.0f, -0.05f, 0.0f), glm::vec3(6.0f, 0.1f, 6.0f), false);

    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(0.0f));
    orbit.zoom(3.0f);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(0.1f, 100.0f);
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();

    LitSettings settings;
    settings.outline = 0.0f;
    settings.colour = glm::vec3(0.45f, 0.55f, 0.9f);
    settings.shadowColour = glm::vec3(0.3f, 0.3f, 0.6f);

    const entt::entity shower = registry.create();
    v3d::ecs::component::Emitter& rain = registry.emplace<v3d::ecs::component::Emitter>(shower, 21u);
    rain.description.direction = glm::vec3(0.0f, -1.0f, 0.0f);
    rain.description.speedMin = 7.0f;
    rain.description.speedMax = 9.0f;
    rain.description.lifeMin = 10.0f;
    rain.description.lifeMax = 10.0f;
    rain.description.cap = 4000;
    rain.description.size = v3d::type::animation::Track<float>(0.03f);
    rain.description.colour = v3d::type::animation::Track<glm::vec4>(glm::vec4(0.6f, 0.7f, 0.9f, 0.5f));
    v3d::render::realtime::component::Particles look;
    look.facing = v3d::render::realtime::component::Particles::Facing::Velocity;
    look.stretch = 0.04f;
    registry.emplace<v3d::render::realtime::component::Particles>(shower, look);

    v3d::type::effect::Weather weather;
    weather.density = 6.0f;
    weather.intensity = 1.0f;
    weather.target = 1.0f;
    weather.wind = glm::vec3(1.5f, 0.0f, 0.0f);

    std::vector<boost::shared_ptr<v3d::image::Image>> frames;
    for (int shot = 0; shot < 3; shot++) {
        for (int stepped = 0; stepped < 30; stepped++) {
            v3d::type::effect::fall(rain.description, &weather, &rain.state, glm::vec3(-3.0f, 0.0f, -3.0f),
                glm::vec3(3.0f, 6.0f, 3.0f), 1.0f / 60.0f);
        }

        boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
            width, height, colourFormat, true);
        Lit lit(headless.device, headless.context->pipelineCache(), headless.context->resources(), headless.context->ring(),
            headless.context->frameUniforms(), headless.context->textures(), colourFormat, target->depthFormat(), VK_FORMAT_UNDEFINED);
        World world(headless.device, headless.context->pipelineCache(), headless.context->resources(),
            headless.context->ring(), headless.context->frameUniforms(), headless.context->textures(), colourFormat,
            target->depthFormat());

        v3d::render::realtime::DepthOrder order;
        const glm::vec3 forward = camera.profile().direction();
        v3d::render::realtime::particles(registry, 1.0f, camera.profile().right(), camera.profile().up(), forward, &order);
        v3d::render::realtime::WorldCanvas drops;
        order.into(&drops);

        Frame frame;
        boost::shared_ptr<Pass> pass = frame.pass("lit");
        pass->target(target);
        pass->depth(true);
        pass->clearColour(glm::vec4(0.02f, 0.02f, 0.05f, 1.0f));
        pass->camera(camera.view(), camera.projection());
        pass->scene(lit.scene(v3d::render::realtime::pack(settings, glm::mat4(1.0f), 0.0f)));
        v3d::render::realtime::meshes(registry, 1.0f, meshes, lit, settings.outline, pass.get());
        world.submit(drops, pass.get(), 0, World::Blend::Additive);

        VkCommandBuffer commands = headless.context->ring()->begin();
        Recorder::record(commands, frame, Recorder::Target(), *headless.context->resources(),
            headless.context->frameUniforms().get());
        frames.push_back(readBack(&headless, commands, target, "data_out/rain_" + std::to_string(shot) + ".png"));
        BOOST_REQUIRE(frames.back());
    }
    BOOST_CHECK(headless.silent());
    BOOST_CHECK(!v3d::image::compare(*frames[0], *frames[1], 0).match);
    BOOST_CHECK(!v3d::image::compare(*frames[1], *frames[2], 0).match);
}

BOOST_AUTO_TEST_SUITE_END()

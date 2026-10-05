/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/Type.h>
#include <api/asset/kind/Model.h>
#include <api/ecs/component/Playback.h>
#include <api/ecs/component/Transform.h>
#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/LitSettings.h>
#include <api/render/realtime/MeshRegistry.h>
#include <api/render/realtime/Meshes.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/Poses.h>
#include <api/render/realtime/SceneUniforms.h>
#include <api/render/realtime/Shadow.h>
#include <api/render/realtime/component/Mesh.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/DepthBuffer.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/renderer/Lit.h>
#include <api/type/Model.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Isometric.h>

#include <cstddef>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <entt/entt.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Headless.h"

using v3d::ecs::component::Playback;
using v3d::ecs::component::Transform;
using v3d::render::realtime::Frame;
using v3d::render::realtime::LitSettings;
using v3d::render::realtime::MeshHandle;
using v3d::render::realtime::MeshRegistry;
using v3d::render::realtime::Pass;
using v3d::render::realtime::Poses;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::vulkan::frame::Recorder;
using v3d::render::realtime::vulkan::frame::RenderTarget;
using v3d::render::realtime::vulkan::renderer::Lit;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_SRGB;
const uint32_t width = 128;
const uint32_t height = 128;

// a strip bound to three joints, with a bend among its clips - api/asset/tests/data/make_skin_fixture.py
const char* STRIP = "bending_strip.glb";

boost::shared_ptr<v3d::asset::Manager> assets(const v3d::test::Headless& headless) {
    return boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless.logger);
}

/**
 * The strip as a static model: the same vertices, parts and materials with its skin taken off.
 **/
v3d::type::Model stripped(const boost::shared_ptr<v3d::asset::Manager>& manager) {
    const boost::shared_ptr<v3d::asset::kind::Model> loaded =
        boost::dynamic_pointer_cast<v3d::asset::kind::Model>(manager->load(STRIP, v3d::asset::Type::ModelGltf));
    BOOST_REQUIRE(loaded && loaded->model());
    v3d::type::Model model = *loaded->model();
    model.skeleton() = v3d::type::Skeleton();
    model.influences().clear();
    model.clips().clear();
    return model;
}

/**
 * An entity drawing a mesh, laid on its back: a quarter turn about x takes the strip's +z face
 * to +y, towards a camera looking down at it.
 **/
entt::entity lay(entt::registry* registry, const MeshHandle& mesh) {
    const entt::entity entity = registry->create();
    Transform transform;
    transform.rotation = glm::angleAxis(-glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
    registry->emplace<Transform>(entity, transform);
    registry->emplace<v3d::render::realtime::component::Mesh>(entity, v3d::render::realtime::component::Mesh{mesh, false});
    return entity;
}

/**
 * Draw a registry's lit entities into a picture, posed, outlined, from above, and read it back.
 **/
boost::shared_ptr<v3d::image::Image> draw(v3d::test::Headless* headless, const entt::registry& registry,
    const MeshRegistry& meshes, Lit* lit, const boost::shared_ptr<RenderTarget>& target, const std::string& path) {
    v3d::type::camera::Isometric orbit;
    // the strip lies from z = 0 to z = -3 once laid down
    orbit.target(glm::vec3(0.0f, 0.0f, -1.5f));
    orbit.zoom(4.0f);
    orbit.elevation(1.0471976f);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(0.1f, 100.0f);
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();

    LitSettings settings;
    settings.outline = 0.05f;

    const Poses poses = v3d::render::realtime::poses(registry, 1.0f, meshes);
    Frame frame(headless->context);
    boost::shared_ptr<Pass> pass = frame.pass("lit");
    pass->target(target);
    pass->depth(true);
    pass->clearColour(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    pass->camera(camera.view(), camera.projection());
    pass->scene(lit->scene(v3d::render::realtime::pack(settings, glm::mat4(1.0f), 0.0f),
        v3d::render::realtime::TextureHandle(), poses.palette()));
    v3d::render::realtime::meshes(registry, 1.0f, meshes, *lit, settings.outline, pass.get(), poses);

    VkCommandBuffer commands = headless->context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless->context->resources(),
        headless->context->frameUniforms().get());

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
 * How many pixels are not the clear colour, so that two blank pictures are not taken for two
 * that agree.
 **/
std::size_t covered(v3d::image::Image& picture) {
    std::size_t count = 0;
    for (std::size_t texel = 0; texel < static_cast<std::size_t>(picture.width()) * picture.height(); texel++) {
        const unsigned char* at = picture.data() + texel * 4;
        if (at[0] != 0 || at[1] != 0 || at[2] != 255) {
            count++;
        }
    }
    return count;
}

std::size_t differing(v3d::image::Image& a, v3d::image::Image& b) {
    std::size_t count = 0;
    for (std::size_t texel = 0; texel < static_cast<std::size_t>(a.width()) * a.height(); texel++) {
        if (std::memcmp(a.data() + texel * 4, b.data() + texel * 4, 4) != 0) {
            count++;
        }
    }
    return count;
}

Lit makeLit(v3d::test::Headless* headless, const boost::shared_ptr<RenderTarget>& target, VkFormat shadow) {
    return Lit(headless->device, headless->context->pipelineCache(), headless->context->resources(), headless->context->ring(),
        headless->context->frameUniforms(), headless->context->quads(), colourFormat, target->depthFormat(), shadow);
}

};  // namespace

BOOST_AUTO_TEST_SUITE(skin_test)

/**
 * Two entities drawing the strip are posed one after the other, each palette as long as the
 * skeleton, and a static entity is not posed at all.
 **/
BOOST_AUTO_TEST_CASE(poses_are_laid_end_to_end) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<v3d::asset::Manager> manager = assets(headless);
    MeshRegistry meshes(headless.logger, headless.context, manager);
    const MeshHandle strip = meshes.load(STRIP);
    const MeshHandle still = meshes.add("still", stripped(manager));

    entt::registry registry;
    const entt::entity first = lay(&registry, strip);
    const entt::entity second = lay(&registry, strip);
    const entt::entity unskinned = lay(&registry, still);

    const Poses poses = v3d::render::realtime::poses(registry, 1.0f, meshes);
    BOOST_CHECK_EQUAL(poses.palette().size(), 6U);
    BOOST_REQUIRE(poses.firstJoint(first) && poses.firstJoint(second));
    BOOST_CHECK_EQUAL(*poses.firstJoint(first) + *poses.firstJoint(second), 3U);
    BOOST_CHECK(!poses.firstJoint(unskinned));
    BOOST_CHECK(headless.silent());
}

/**
 * An entity with no playback, and one playing nothing, stand in the rest pose, whose palette
 * is the identity; one whose mesh was released is not posed.
 **/
BOOST_AUTO_TEST_CASE(an_entity_with_nothing_playing_stands_at_rest) {
    v3d::test::Headless headless(colourFormat, width, height);
    MeshRegistry meshes(headless.logger, headless.context, assets(headless));
    const MeshHandle strip = meshes.load(STRIP);
    const MeshHandle doomed = meshes.add("doomed", *boost::dynamic_pointer_cast<v3d::asset::kind::Model>(
        assets(headless)->load(STRIP, v3d::asset::Type::ModelGltf))->model());

    entt::registry registry;
    lay(&registry, strip);
    const entt::entity idle = lay(&registry, strip);
    registry.emplace<Playback>(idle);
    const entt::entity released = lay(&registry, doomed);
    BOOST_REQUIRE(meshes.release(doomed));

    const Poses poses = v3d::render::realtime::poses(registry, 1.0f, meshes);
    BOOST_CHECK(!poses.firstJoint(released));
    BOOST_REQUIRE_EQUAL(poses.palette().size(), 6U);
    for (const glm::mat4& matrix : poses.palette()) {
        BOOST_CHECK(matrix == glm::mat4(1.0f));
    }
    BOOST_CHECK(headless.silent());
}

/**
 * At its rest pose a skin draws the picture its unskinned mesh draws, byte for byte: the rest
 * palette is exactly the identity and every weight is exact, so skinning changes no vertex.
 * This is what fails on a wrong stride, a joint read from the wrong attribute, a wrong first
 * joint, or a palette that never reached the set.
 *
 * The pictures are written to data_out/skinned_rest.png and data_out/unskinned_rest.png.
 **/
BOOST_AUTO_TEST_CASE(a_skin_at_rest_draws_its_unskinned_mesh) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, colourFormat, true);
    const boost::shared_ptr<v3d::asset::Manager> manager = assets(headless);
    MeshRegistry meshes(headless.logger, headless.context, manager);
    Lit lit = makeLit(&headless, target, VK_FORMAT_UNDEFINED);

    entt::registry skinned;
    lay(&skinned, meshes.load(STRIP));
    entt::registry unskinned;
    lay(&unskinned, meshes.add("stripped", stripped(manager)));

    const boost::shared_ptr<v3d::image::Image> posed = draw(&headless, skinned, meshes, &lit, target, "data_out/skinned_rest.png");
    const boost::shared_ptr<v3d::image::Image> still = draw(&headless, unskinned, meshes, &lit, target, "data_out/unskinned_rest.png");
    BOOST_REQUIRE(posed && still);
    BOOST_CHECK(headless.silent());

    BOOST_TEST_MESSAGE("covered " << covered(*still) << ", differing " << differing(*posed, *still));
    BOOST_CHECK_GT(covered(*still), 500U);
    BOOST_CHECK_EQUAL(differing(*posed, *still), 0U);
}

/**
 * The strip bent half way through its clip is drawn, silently, and is not the strip at rest.
 *
 * The picture is written to data_out/skinned_bend.png for a person to look at.
 **/
BOOST_AUTO_TEST_CASE(a_bent_strip_is_drawn_and_silent) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, colourFormat, true);
    MeshRegistry meshes(headless.logger, headless.context, assets(headless));
    Lit lit = makeLit(&headless, target, VK_FORMAT_UNDEFINED);
    const MeshHandle strip = meshes.load(STRIP);
    const std::optional<uint32_t> bend = meshes.clip(strip, "bend");
    BOOST_REQUIRE(bend);
    BOOST_CHECK(!meshes.clip(strip, "no such clip"));

    entt::registry registry;
    const entt::entity entity = lay(&registry, strip);
    const boost::shared_ptr<v3d::image::Image> rested = draw(&headless, registry, meshes, &lit, target, "data_out/skinned_rest.png");

    Playback playback;
    v3d::ecs::component::play(&playback, *bend, meshes.resolve(strip)->skin->clips[*bend].duration, false, 0.0f);
    v3d::ecs::component::advance(&playback, 0.5f);
    registry.emplace<Playback>(entity, playback);
    const boost::shared_ptr<v3d::image::Image> bent = draw(&headless, registry, meshes, &lit, target, "data_out/skinned_bend.png");
    BOOST_REQUIRE(rested && bent);
    BOOST_CHECK(headless.silent());

    BOOST_TEST_MESSAGE("bending moved " << differing(*rested, *bent) << " pixels");
    BOOST_CHECK_GT(differing(*rested, *bent), 100U);
}

/**
 * A rig Blender built and exported - make_blender_fixture.py - drawn at the start of its bend
 * and a third and two thirds through. Each frame is silent, and each moves the strip from the
 * one before. The strip bends towards the camera here, so the change is foreshortening.
 *
 * The pictures are written to data_out/blender_bend_0.png, _1 and _2 for a person to look at.
 **/
BOOST_AUTO_TEST_CASE(an_exported_rig_is_drawn_mid_clip) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, colourFormat, true);
    MeshRegistry meshes(headless.logger, headless.context, assets(headless));
    Lit lit = makeLit(&headless, target, VK_FORMAT_UNDEFINED);
    const MeshHandle rig = meshes.load("blender_strip.glb");
    const std::optional<uint32_t> bend = meshes.clip(rig, "bend");
    BOOST_REQUIRE(bend);
    const float duration = meshes.resolve(rig)->skin->clips[*bend].duration;

    entt::registry registry;
    const entt::entity entity = lay(&registry, rig);
    Playback playback;
    v3d::ecs::component::play(&playback, *bend, duration, false, 0.0f);
    registry.emplace<Playback>(entity, playback);

    std::vector<boost::shared_ptr<v3d::image::Image>> pictures;
    for (int third = 0; third < 3; third++) {
        pictures.push_back(draw(&headless, registry, meshes, &lit, target, "data_out/blender_bend_" + std::to_string(third) + ".png"));
        BOOST_REQUIRE(pictures.back());
        v3d::ecs::component::advance(registry, duration / 3.0f);
    }
    BOOST_CHECK(headless.silent());

    BOOST_CHECK_GT(covered(*pictures[0]), 500U);
    for (std::size_t frame = 1; frame < pictures.size(); frame++) {
        BOOST_TEST_MESSAGE("frame " << frame << " moved " << differing(*pictures[frame - 1], *pictures[frame]) << " pixels");
        BOOST_CHECK_GT(differing(*pictures[frame - 1], *pictures[frame]), 50U);
    }
}

/**
 * A skinned caster is drawn into the shadow map in its pose, not the one it was bound in. A
 * square of one joint faces the light at z = -1, which is a quarter of the way into the light's
 * range, and the joint stands half a unit nearer the far plane, so the square is cast at
 * z = -0.5: three eighths. Cast in its bind pose it would read a quarter.
 **/
BOOST_AUTO_TEST_CASE(a_skinned_caster_casts_its_pose) {
    const uint32_t size = 16;
    v3d::test::Headless headless(colourFormat, size, size);
    boost::shared_ptr<RenderTarget> map = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        size, size, VK_FORMAT_UNDEFINED, true, true);
    if (map->depthFormat() != VK_FORMAT_D32_SFLOAT) {
        BOOST_TEST_MESSAGE("The device gives no sampled D32_SFLOAT, so there is no exact depth to compare");
        return;
    }

    // a square at z = -0.5 facing -z, towards the light, wound counter clockwise from there
    v3d::type::Model square;
    const glm::vec3 corners[4] = {{0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}};
    for (const glm::vec3& corner : corners) {
        v3d::type::Model::Vertex vertex;
        vertex.position = corner;
        vertex.normal = glm::vec3(0.0f, 0.0f, -1.0f);
        square.vertices().push_back(vertex);
        v3d::type::Model::Influence influence;
        influence.weights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        square.influences().push_back(influence);
    }
    square.indices() = {0, 1, 2, 0, 2, 3};
    square.materials().emplace_back();
    square.parts().push_back({0, 6, 0});
    v3d::type::Skeleton::Joint joint;
    joint.translation = glm::vec3(0.0f, 0.0f, 0.5f);
    square.skeleton().joints.push_back(joint);

    MeshRegistry meshes(headless.logger, headless.context, assets(headless));
    const MeshHandle handle = meshes.add("square", square);
    Lit lit(headless.device, headless.context->pipelineCache(), headless.context->resources(), headless.context->ring(),
        headless.context->frameUniforms(), headless.context->quads(), colourFormat,
        v3d::render::realtime::vulkan::frame::DepthBuffer::chooseFormat(headless.device->physical()), map->depthFormat());

    entt::registry registry;
    const entt::entity entity = registry.create();
    Transform transform;
    transform.position = glm::vec3(0.0f, 0.0f, -0.5f);
    registry.emplace<Transform>(entity, transform);
    registry.emplace<v3d::render::realtime::component::Mesh>(entity, v3d::render::realtime::component::Mesh{handle, true});

    // the light looks along +z from two units out over a sphere of one, as the static case's
    const glm::mat4 light = v3d::render::realtime::shadow::light(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f), 1.0f);
    const Poses poses = v3d::render::realtime::poses(registry, 1.0f, meshes);

    Frame frame(headless.context);
    boost::shared_ptr<Pass> pass = frame.pass("shadow");
    pass->target(map);
    pass->depth(true);
    pass->scene(lit.scene(v3d::render::realtime::pack(LitSettings(), light, 1.0f / size),
        v3d::render::realtime::TextureHandle(), poses.palette()));
    pass->depthBias(0.0f, 0.0f);
    v3d::render::realtime::casters(registry, 1.0f, meshes, lit, pass.get(), poses);
    BOOST_CHECK_EQUAL(pass->items().size(), 1U);

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
    BOOST_CHECK_EQUAL(depths[static_cast<std::size_t>(8) * size + 8], 0.375f);
    BOOST_CHECK_EQUAL(depths[0], 1.0f);
}

/**
 * A skinned mesh released while a frame draws it keeps the frame silent, as a static one does.
 **/
BOOST_AUTO_TEST_CASE(a_skinned_mesh_released_in_flight_outlives_its_frame) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, colourFormat, true);
    MeshRegistry meshes(headless.logger, headless.context, assets(headless));
    Lit lit = makeLit(&headless, target, VK_FORMAT_UNDEFINED);
    const MeshHandle strip = meshes.load(STRIP);

    entt::registry registry;
    lay(&registry, strip);
    const Poses poses = v3d::render::realtime::poses(registry, 1.0f, meshes);
    Frame frame(headless.context);
    boost::shared_ptr<Pass> pass = frame.pass("lit");
    pass->target(target);
    pass->depth(true);
    pass->scene(lit.scene(v3d::render::realtime::pack(LitSettings(), glm::mat4(1.0f), 0.0f),
        v3d::render::realtime::TextureHandle(), poses.palette()));
    v3d::render::realtime::meshes(registry, 1.0f, meshes, lit, 0.0f, pass.get(), poses);

    VkCommandBuffer commands = headless.context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless.context->resources(),
        headless.context->frameUniforms().get());
    headless.submit(commands);

    BOOST_CHECK(meshes.release(strip));
    for (uint32_t turn = 0; turn <= headless.context->ring()->framesInFlight(); turn++) {
        headless.submit(headless.context->ring()->begin());
    }
    headless.context->ring()->waitIdle();
    BOOST_CHECK(headless.silent());
}

BOOST_AUTO_TEST_SUITE_END()

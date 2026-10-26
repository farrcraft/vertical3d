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
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/canvas/DepthOrder.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Grade.h>
#include <api/render/realtime/lighting/Settings.h>
#include <api/render/realtime/MeshRegistry.h>
#include <api/render/realtime/entity/Particles.h>
#include <api/render/realtime/entity/Meshes.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/lighting/SceneUniforms.h>
#include <api/render/realtime/lighting/Shadow.h>
#include <api/render/realtime/canvas/World.h>
#include <api/render/realtime/component/Mesh.h>
#include <api/render/realtime/component/Particles.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/DepthBuffer.h>
#include <api/render/realtime/Recorder.h>
#include <api/render/realtime/RenderTarget.h>
#include <api/render/realtime/renderer/Lit.h>
#include <api/render/realtime/renderer/World.h>
#include <api/type/animation/Track.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Isometric.h>
#include <api/type/effect/Weather.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <entt/entt.hpp>

#include "Headless.h"
#include "Shapes.h"

using v3d::ecs::component::Transform;
using v3d::render::realtime::Frame;
using v3d::render::realtime::MeshHandle;
using v3d::render::realtime::MeshRegistry;
using v3d::render::realtime::Pass;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::Recorder;
using v3d::render::realtime::RenderTarget;
using v3d::render::realtime::renderer::Lit;
using v3d::render::realtime::renderer::World;
using v3d::test::cube;
using v3d::test::twoCubes;

namespace lighting = v3d::render::realtime::lighting;

namespace {

// the lit pipelines compute in linear light, and an SRGB target encodes it on store
const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_SRGB;
const uint32_t width = 128;
const uint32_t height = 128;

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
 * How the shadow changed a picture: the pixels that are white ground without it and grey with
 * it, and the pixels that changed in any other way. A white surface in shadow drops from the lit
 * band to the mid one, and its edge blends the two, so it stays grey and gets no lighter.
 **/
struct Shading final {
    std::size_t darkened = 0;
    std::size_t otherwise = 0;
};

/** The same scene drawn without a change and with it. **/
struct Drawn {
    boost::shared_ptr<v3d::image::Image> without;
    boost::shared_ptr<v3d::image::Image> with;
};

Shading compare(const Drawn& drawn) {
    const boost::shared_ptr<v3d::image::Image>& without = drawn.without;
    const boost::shared_ptr<v3d::image::Image>& with = drawn.with;
    Shading shading;
    for (std::size_t texel = 0; texel < static_cast<std::size_t>(with->width()) * with->height(); texel++) {
        const unsigned char* before = without->data() + (texel * 4);
        const unsigned char* after = with->data() + (texel * 4);
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
 * @param strength lighting::Settings::shadowStrength, so a case can draw the same frame with the
 *        shadow ignored
 * @param frames how many frames to draw into the same shadow map. Every frame but the last is
 *        submitted without waiting, as a presented frame is, so two of them are in flight at once
 **/
boost::shared_ptr<v3d::image::Image> drawShadowed(v3d::test::Headless* headless, float strength, const std::string& path,
    int frames = 1) {
    const uint32_t mapSize = 256;
    const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(*headless->context,
        RenderTarget::Spec{.width = width, .height = height, .colour = colourFormat, .depth = true});
    const boost::shared_ptr<RenderTarget> map = boost::make_shared<RenderTarget>(*headless->context,
        RenderTarget::Spec{.width = mapSize, .height = mapSize, .colour = VK_FORMAT_UNDEFINED, .depth = true, .sampledDepth = true});

    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless->logger);

    v3d::asset::media::registerLoaders(*assets, headless->logger);
    MeshRegistry meshes(headless->logger, headless->context, assets);
    const MeshHandle crate = meshes.add("crate", cube(glm::vec4(1.0F, 0.0F, 0.0F, 1.0F)));
    const MeshHandle ground = meshes.add("ground", cube(glm::vec4(1.0F)));

    Lit lit(*headless->context, Lit::Spec{.colour = colourFormat, .depth = target->depthFormat(), .shadow = map->depthFormat()});

    entt::registry registry;
    place(&registry, crate, glm::vec3(0.0F, 0.5F, 0.0F), glm::vec3(1.0F), true);
    // thin, and topped at zero, so the cube stands on it. It casts nothing, so the fit is the
    // cube's alone
    place(&registry, ground, glm::vec3(0.0F, -0.05F, 0.0F), glm::vec3(6.0F, 0.1F, 6.0F), false);

    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(0.0F));
    orbit.zoom(3.0F);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(glm::vec2(0.1F, 100.0F));
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();

    lighting::Settings settings;
    settings.outline = 0.0F;
    settings.shadowStrength = strength;

    const std::optional<lighting::ShadowBounds> bounds =
        lighting::fitShadow(registry, {}, {.margin = 2.0F});
    if (!bounds) {
        BOOST_ERROR("a cube that casts gave no fit");
        return boost::shared_ptr<v3d::image::Image>();
    }
    const glm::mat4 light = lighting::shadowLight(settings.light, *bounds);
    VkDescriptorSet scene = lit.scene(lighting::pack(settings, light, 1.0F / mapSize),
        headless->context->textures()->depthTexture(*map));

    for (int drawn = 1; ; drawn++) {
        // the lit pass is made first, as an engine's colour pass is, and reads the map, so the
        // frame records the shadow pass ahead of it
        Frame frame;
        const boost::shared_ptr<Pass> pass = frame.pass("lit");
        const boost::shared_ptr<Pass> casting = frame.pass("shadow");
        casting->target(map);
        casting->depth(true);
        casting->scene(scene);
        casting->depthBias(settings.constantBias, settings.slopeBias);
        v3d::render::realtime::entity::casters(registry, 1.0F, meshes, &lit, casting.get());

        pass->reads(map);
        pass->target(target);
        pass->depth(true);
        pass->clearColour(glm::vec4(0.0F, 0.0F, 1.0F, 1.0F));
        pass->camera(camera.view(), camera.projection());
        pass->scene(scene);
        v3d::render::realtime::entity::meshes(registry, 1.0F, meshes, &lit, settings.outline, pass.get());

        if (drawn == frames) {
            return headless->picture(frame, *target, path);
        }
        VkCommandBuffer commands = headless->context->ring()->begin();
        Recorder::record(commands, frame, Recorder::Target(), *headless->context->resources(),
            headless->context->frameUniforms().get());
        headless->submit(commands);
    }
}

/**
 * A lit red cube, outlined, drawn through the recorder from an entity, and the checks the
 * picture has to pass. The pixel at the centre lands on the cube's top face, which faces the
 * key light and so is in the lit band of the cube's own colour. The same pixel checks the
 * winding. With the outline's faces the wrong way round, the near side of its hull covers the
 * pixel in black. With the cel pass's faces the wrong way round, the inside of the bottom face
 * shows there instead.
 *
 * @param hand the camera's hand
 * @param front the front face Lit is built with
 * @param path where the picture is written, for a person to look at
 **/
void drawLitCube(v3d::test::Headless* headless, v3d::type::camera::Profile::Hand hand, VkFrontFace front,
    const std::string& path) {
    const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(*headless->context,
        RenderTarget::Spec{.width = width, .height = height, .colour = colourFormat, .depth = true});

    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless->logger);

    v3d::asset::media::registerLoaders(*assets, headless->logger);
    MeshRegistry meshes(headless->logger, headless->context, assets);
    const MeshHandle crate = meshes.add("crate", cube(glm::vec4(1.0F, 0.0F, 0.0F, 1.0F)));

    Lit lit(*headless->context,
        Lit::Spec{.colour = colourFormat, .depth = target->depthFormat(), .shadow = VK_FORMAT_UNDEFINED, .front = front});

    entt::registry registry;
    const entt::entity entity = registry.create();
    registry.emplace<Transform>(entity);
    registry.emplace<v3d::render::realtime::component::Mesh>(entity, v3d::render::realtime::component::Mesh{crate, true});

    // type::camera's orthographic camera, which builds Vulkan clip space and so decides which
    // way round a face is on screen. Steeper than its default 45 degrees, so that the ray
    // through the centre of the picture meets the top face rather than an edge
    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(0.0F));
    orbit.zoom(1.5F);
    orbit.elevation(1.0471976F);
    orbit.hand(hand);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(glm::vec2(0.1F, 100.0F));
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();

    // thicker than the default, which at this zoom is under a pixel and can cover no pixel
    // centre at all
    lighting::Settings settings;
    settings.outline = 0.1F;

    // the frame is built before it is begun, as an engine builds one during its tick. A scene
    // waits for its slot to be free, and a begun frame's fence is not signalled until it is
    // submitted
    Frame frame;
    const boost::shared_ptr<Pass> pass = frame.pass("lit");
    pass->target(target);
    pass->depth(true);
    pass->clearColour(glm::vec4(0.0F, 0.0F, 1.0F, 1.0F));
    pass->camera(camera.view(), camera.projection());
    pass->scene(lit.scene(lighting::pack(settings, glm::mat4(1.0F), 0.0F)));
    v3d::render::realtime::entity::meshes(registry, 1.0F, meshes, &lit, settings.outline, pass.get());

    // the pass names its own target, so draw() gives the frame no image
    Capture capture(headless->device, headless->context->ring(), headless->logger);
    headless->draw(frame, *target, &capture);

    BOOST_CHECK(headless->silent());
    BOOST_REQUIRE(capture.write(path));

    v3d::image::reader::Png png(headless->logger);
    const boost::shared_ptr<v3d::image::Image> picture = png.read(path);
    BOOST_REQUIRE(picture);
    const unsigned char* centre = picture->data() + (((static_cast<std::size_t>(height / 2) * width) + (width / 2)) * 4);
    BOOST_TEST_MESSAGE("centre " << int(centre[0]) << "," << int(centre[1]) << "," << int(centre[2]));
    BOOST_CHECK_GT(centre[0], 200);
    BOOST_CHECK_LT(centre[1], 10);
    BOOST_CHECK_LT(centre[2], 10);

    // the outline is black and nothing else in the picture is, so a black pixel is one
    std::size_t outlined = 0;
    for (std::size_t texel = 0; texel < static_cast<std::size_t>(width) * height; texel++) {
        const unsigned char* at = picture->data() + (texel * 4);
        if (at[0] == 0 && at[1] == 0 && at[2] == 0) {
            outlined++;
        }
    }
    BOOST_TEST_MESSAGE("outline pixels " << outlined);
    BOOST_CHECK_GT(outlined, 0U);
}

/**
 * A caster is drawn into a shadow map at the depth the light's matrix gives it. This is the
 * depth target's case again, with the quads standing in the world and drawn through Lit's
 * shadow pipeline and lighting::shadowLight. The depths are exact for the same reason: each front
 * face is a plane of one depth, at a quarter and three quarters of the light's range.
 *
 * A third entity that casts no shadow stands nearer the light over the right quad. If it were
 * drawn into the map, it would put its own depth there.
 *
 * The depth stored is the face nearest the light. With the shadow pipeline culling the wrong
 * way round, it would be the far face of each block instead.
 *
 * @param hand the hand the light's matrix is built in
 * @param front the front face Lit is built with
 **/
void checkCasterDepths(v3d::type::camera::Profile::Hand hand, VkFrontFace front) {
    const uint32_t size = 16;
    v3d::test::Headless headless(colourFormat, size, size);
    const boost::shared_ptr<RenderTarget> map = boost::make_shared<RenderTarget>(*headless.context,
        RenderTarget::Spec{.width = size, .height = size, .colour = VK_FORMAT_UNDEFINED, .depth = true, .sampledDepth = true});
    if (map->depthFormat() != VK_FORMAT_D32_SFLOAT) {
        BOOST_TEST_MESSAGE("The device gives no sampled D32_SFLOAT, so there is no exact depth to compare");
        return;
    }

    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless.logger);

    v3d::asset::media::registerLoaders(*assets, headless.logger);
    MeshRegistry meshes(headless.logger, headless.context, assets);
    const MeshHandle block = meshes.add("block", cube(glm::vec4(1.0F)));
    Lit lit(*headless.context,
        Lit::Spec{.colour = colourFormat,
            .depth = v3d::render::realtime::vulkan::frame::DepthBuffer::chooseFormat(headless.device->physical()),
            .shadow = map->depthFormat(), .front = front});

    // the light looks along +z from two units out, over a sphere of one. A face at z = -1 is
    // therefore a quarter of the way into its range, and one at z = 1 three quarters. Each
    // block is half a unit wide and spans the same rectangles as the depth target's two quads
    entt::registry registry;
    const glm::vec3 scale(0.5F, 1.0F, 1.0F);
    place(&registry, block, glm::vec3(-0.5F, 0.0F, -0.5F), scale, true);
    place(&registry, block, glm::vec3(0.5F, 0.0F, 1.5F), scale, true);
    place(&registry, block, glm::vec3(0.5F, 0.0F, 0.5F), scale, false);

    const glm::mat4 light = lighting::shadowLight(glm::vec3(0.0F, 0.0F, -1.0F), {.centre = glm::vec3(0.0F), .radius = 1.0F}, hand);

    Frame frame;
    const boost::shared_ptr<Pass> pass = frame.pass("shadow");
    pass->target(map);
    pass->depth(true);
    pass->scene(lit.scene(lighting::pack(lighting::Settings(), light, 1.0F / size)));
    // the pipeline is biased, so the pass must name a bias. It names zero, so the depths stay
    // exact
    pass->depthBias(0.0F, 0.0F);
    v3d::render::realtime::entity::casters(registry, 1.0F, meshes, &lit, pass.get());
    // the two that cast share a mesh, so they are one draw of two instances
    BOOST_CHECK_EQUAL(pass->items().size(), 1U);

    Capture capture(headless.device, headless.context->ring(), headless.logger);
    headless.draw(frame, *map, &capture, v3d::test::Headless::Read::Depth);

    BOOST_CHECK(headless.silent());
    const std::vector<float> depths = capture.depth();
    BOOST_REQUIRE_EQUAL(depths.size(), static_cast<std::size_t>(size) * size);
    const auto at = [&depths, size](uint32_t x, uint32_t y) { return depths[(static_cast<std::size_t>(y) * size) + x]; };
    // the mirrored hand mirrors the map, so the two blocks change sides
    const bool mirrored = hand == v3d::type::camera::Profile::Hand::DirectionCrossUp;
    BOOST_CHECK_EQUAL(at(mirrored ? 11 : 4, 8), 0.25F);
    BOOST_CHECK_EQUAL(at(mirrored ? 4 : 11, 8), 0.75F);
    BOOST_CHECK_EQUAL(at(8, 8), 1.0F);
    BOOST_CHECK_EQUAL(at(0, 0), 1.0F);
}

BOOST_AUTO_TEST_SUITE(lit_scene_test)

/**
 * A lit cube, outlined, drawn through the recorder from an entity under the default camera.
 * Lighting arithmetic is left to the implementation, so there is no reference picture. The case
 * checks that the validation layer reports no errors, and checks the pixels drawLitCube names.
 *
 * The picture is always written to data_out/lit_cube.png for a person to look at.
 **/
BOOST_AUTO_TEST_CASE(a_lit_entity_is_drawn_and_silent) {
    v3d::test::Headless headless(colourFormat, width, height);
    drawLitCube(&headless, v3d::type::camera::Profile::Hand::UpCrossDirection, VK_FRONT_FACE_CLOCKWISE,
        "data_out/lit_cube.png");
}

/**
 * The same cube under a camera on the mirrored hand, which glm::lookAt builds, drawn by a Lit
 * whose front face is counter clockwise. The image is mirrored, so a Lit left at clockwise would
 * cull the faces that face the camera and draw the outline hull's near side.
 *
 * The picture is written to data_out/lit_cube_mirrored.png for a person to look at.
 **/
BOOST_AUTO_TEST_CASE(a_lit_entity_is_drawn_under_the_mirrored_hand) {
    v3d::test::Headless headless(colourFormat, width, height);
    drawLitCube(&headless, v3d::type::camera::Profile::Hand::DirectionCrossUp, VK_FRONT_FACE_COUNTER_CLOCKWISE,
        "data_out/lit_cube_mirrored.png");
}

/**
 * Creating Lit's pipelines leaves the validation layer with nothing to say, at warning severity
 * as well as at error. A pipeline that declares a vertex attribute its shader does not read is
 * a warning, not an error, so a case that counts errors alone passes over it. Every pipeline is
 * built here, the shadow casters included, in both the plain and the skinned form.
 **/
BOOST_AUTO_TEST_CASE(lit_pipelines_are_created_without_a_warning) {
    const v3d::test::Headless headless(colourFormat, width, height);
    BOOST_REQUIRE(headless.instance->validating());
    const uint32_t before = headless.instance->warnings();

    const VkFormat depth = v3d::render::realtime::vulkan::frame::DepthBuffer::chooseFormat(headless.device->physical());
    const Lit lit(*headless.context,
        Lit::Spec{.colour = colourFormat, .depth = depth, .shadow = depth, .front = VK_FRONT_FACE_CLOCKWISE});

    BOOST_CHECK_EQUAL(headless.instance->warnings() - before, 0U);
    BOOST_CHECK(headless.silent());
}

/**
 * A pass that turns depth writing off reaches only the pipelines that allow it. A near red cube
 * is drawn first and a far green one after it, straight behind it, in a pass with depth writing
 * off. Lit's pipelines write depth whatever the pass says, so the near cube still hides the far
 * one and the centre is red. Were the pass to reach them, the far cube would pass the depth test
 * against the clear value and draw over the near one.
 *
 * The two cubes are submitted from two registries, one after the other, so the near one is
 * drawn first whatever order a registry iterates its entities in.
 **/
BOOST_AUTO_TEST_CASE(a_pass_without_depth_writes_leaves_lit_writing) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(*headless.context,
        RenderTarget::Spec{.width = width, .height = height, .colour = colourFormat, .depth = true});

    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless.logger);
    v3d::asset::media::registerLoaders(*assets, headless.logger);
    MeshRegistry meshes(headless.logger, headless.context, assets);
    const MeshHandle red = meshes.add("red", cube(glm::vec4(1.0F, 0.0F, 0.0F, 1.0F)));
    const MeshHandle green = meshes.add("green", cube(glm::vec4(0.0F, 1.0F, 0.0F, 1.0F)));

    Lit lit(*headless.context, Lit::Spec{.colour = colourFormat, .depth = target->depthFormat(), .shadow = VK_FORMAT_UNDEFINED});

    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(0.0F));
    orbit.zoom(3.0F);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(glm::vec2(0.1F, 100.0F));
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();

    // the far cube is three units further along the view, so an orthographic camera draws it
    // exactly behind the near one
    entt::registry nearer;
    entt::registry farther;
    place(&nearer, red, glm::vec3(0.0F), glm::vec3(1.0F), false);
    place(&farther, green, camera.profile().direction() * 3.0F, glm::vec3(1.0F), false);

    lighting::Settings settings;
    settings.outline = 0.0F;

    Frame frame;
    const boost::shared_ptr<Pass> pass = frame.pass("lit");
    pass->target(target);
    pass->depth(true);
    pass->depthWrite(false);
    pass->clearColour(glm::vec4(0.0F, 0.0F, 1.0F, 1.0F));
    pass->camera(camera.view(), camera.projection());
    pass->scene(lit.scene(lighting::pack(settings, glm::mat4(1.0F), 0.0F)));
    v3d::render::realtime::entity::meshes(nearer, 1.0F, meshes, &lit, settings.outline, pass.get());
    v3d::render::realtime::entity::meshes(farther, 1.0F, meshes, &lit, settings.outline, pass.get());

    const boost::shared_ptr<v3d::image::Image> picture = headless.picture(frame, *target, "data_out/lit_unwritten_pass.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());

    const unsigned char* centre = picture->data() + (((static_cast<std::size_t>(height / 2) * width) + (width / 2)) * 4);
    BOOST_TEST_MESSAGE("centre " << int(centre[0]) << "," << int(centre[1]) << "," << int(centre[2]));
    BOOST_CHECK_GT(centre[0], 0);
    BOOST_CHECK_EQUAL(centre[1], 0);
}

/**
 * An entity whose mesh was released is skipped rather than drawn from a stale entry, so
 * meshes() submits nothing for it and nothing for its outline.
 **/
BOOST_AUTO_TEST_CASE(a_released_mesh_is_not_walked) {
    const v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless.logger);
    v3d::asset::media::registerLoaders(*assets, headless.logger);
    MeshRegistry meshes(headless.logger, headless.context, assets);
    Lit lit(*headless.context,
        Lit::Spec{.colour = colourFormat,
            .depth = v3d::render::realtime::vulkan::frame::DepthBuffer::chooseFormat(headless.device->physical()),
            .shadow = VK_FORMAT_UNDEFINED});

    entt::registry registry;
    const MeshHandle kept = meshes.add("kept", cube(glm::vec4(1.0F)));
    const MeshHandle released = meshes.add("released", cube(glm::vec4(1.0F)));
    for (const MeshHandle& handle : {kept, released}) {
        const entt::entity entity = registry.create();
        registry.emplace<Transform>(entity);
        registry.emplace<v3d::render::realtime::component::Mesh>(entity, v3d::render::realtime::component::Mesh{handle, true});
    }
    BOOST_REQUIRE(meshes.release(released));

    // meshes() adds each draw's object to the frame's scene, so the scene is written first
    Pass pass("lit");
    pass.scene(lit.scene(lighting::pack(lighting::Settings(), glm::mat4(1.0F), 0.0F)));
    v3d::render::realtime::entity::meshes(registry, 1.0F, meshes, &lit, 0.1F, &pass);

    // an outline and a surface for the one that is left
    BOOST_CHECK_EQUAL(pass.items().size(), 2U);
}

/**
 * A caster is drawn into a shadow map at the depth the light's matrix gives it, under the
 * default hand: checkCasterDepths.
 **/
BOOST_AUTO_TEST_CASE(a_caster_is_drawn_into_the_shadow_map_at_its_depth) {
    checkCasterDepths(v3d::type::camera::Profile::Hand::UpCrossDirection, VK_FRONT_FACE_CLOCKWISE);
}

/**
 * The same casters through a light built in the mirrored hand, by a Lit whose front face is
 * counter clockwise. A shadow pipeline left at clockwise would store each block's far face.
 **/
BOOST_AUTO_TEST_CASE(a_caster_is_drawn_into_the_shadow_map_under_the_mirrored_hand) {
    checkCasterDepths(v3d::type::camera::Profile::Hand::DirectionCrossUp, VK_FRONT_FACE_COUNTER_CLOCKWISE);
}

/**
 * Two frames in flight share one shadow map. The second frame's shadow pass writes the map
 * while the first frame's lit pass may still be sampling it, so the barrier before the shadow
 * pass writes the map has to wait for that read. Only synchronization validation reports a
 * barrier that does not wait, so the case checks that the layer is silent.
 **/
BOOST_AUTO_TEST_CASE(a_shadow_map_is_shared_by_frames_in_flight) {
    v3d::test::Headless headless(colourFormat, width, height);

    const boost::shared_ptr<v3d::image::Image> picture = drawShadowed(&headless, 1.0F, "data_out/lit_shadow_twice.png", 2);
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

    const boost::shared_ptr<v3d::image::Image> ignored = drawShadowed(&headless, 0.0F, "data_out/lit_unshadowed.png");
    BOOST_REQUIRE(ignored);
    const boost::shared_ptr<v3d::image::Image> picture = drawShadowed(&headless, 1.0F, "data_out/lit_shadow.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());

    const Shading shading = compare({.without = ignored, .with = picture});
    BOOST_TEST_MESSAGE("ground darkened " << shading.darkened << ", anything else changed " << shading.otherwise);
    BOOST_CHECK_GT(shading.darkened, 0U);
    BOOST_CHECK_EQUAL(shading.otherwise, 0U);
}

/**
 * A full-size lit game scene built from the asset suite's fixtures. It holds a ground that casts
 * nothing, two upright figures and ten props at set yaws, under one key light at the defaults of
 * lighting::Settings. It is seen through an orthographic camera at 45 degrees at 1280 by 720.
 * The frame is a shadow pass, the lit scene into an sRGB target, and a grade into a target in
 * an sRGB swapchain format. The grade uses the identity table.
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
    const boost::shared_ptr<RenderTarget> scene = boost::make_shared<RenderTarget>(*headless.context,
        RenderTarget::Spec{.width = frameWidth, .height = frameHeight, .colour = swapchainFormat, .depth = true});
    const boost::shared_ptr<RenderTarget> map = boost::make_shared<RenderTarget>(*headless.context,
        RenderTarget::Spec{.width = mapSize, .height = mapSize, .colour = VK_FORMAT_UNDEFINED, .depth = true, .sampledDepth = true});
    const boost::shared_ptr<RenderTarget> output = boost::make_shared<RenderTarget>(*headless.context,
        RenderTarget::Spec{.width = frameWidth, .height = frameHeight, .colour = swapchainFormat});

    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless.logger);

    v3d::asset::media::registerLoaders(*assets, headless.logger);
    MeshRegistry meshes(headless.logger, headless.context, assets);
    const MeshHandle ground = meshes.add("ground", cube(glm::vec4(0.55F, 0.6F, 0.5F, 1.0F)));
    const MeshHandle figure = meshes.add("figure", cube(glm::vec4(0.8F, 0.3F, 0.2F, 1.0F)));
    const MeshHandle crate = meshes.add("crate", cube(glm::vec4(0.6F, 0.45F, 0.3F, 1.0F)));
    const MeshHandle primitives = meshes.load("three_primitives.glb");
    const MeshHandle textured = meshes.load("embedded_texture.glb");
    BOOST_REQUIRE(meshes.resolve(primitives) != nullptr);
    BOOST_REQUIRE(meshes.resolve(textured) != nullptr);

    Lit lit(*headless.context, Lit::Spec{.colour = swapchainFormat, .depth = scene->depthFormat(), .shadow = map->depthFormat()});
    v3d::render::realtime::Grade grade(*headless.context, v3d::render::realtime::Grade::Spec{.colour = swapchainFormat});
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
    stand(ground, glm::vec3(0.0F, -0.05F, 0.0F), 0.0F, glm::vec3(14.0F, 0.1F, 14.0F), false);
    stand(figure, glm::vec3(-1.0F, 0.9F, 0.5F), 0.3F, glm::vec3(0.6F, 1.8F, 0.4F), true);
    stand(figure, glm::vec3(1.2F, 0.9F, -0.4F), -0.8F, glm::vec3(0.6F, 1.8F, 0.4F), true);
    // ten props, in a ring around the two, each turned its own way
    const MeshHandle props[3] = {crate, primitives, textured};
    for (int prop = 0; prop < 10; prop++) {
        const float around = static_cast<float>(prop) * 0.6283185F;
        const glm::vec3 at(std::cos(around) * 4.0F, 0.5F, std::sin(around) * 4.0F);
        stand(props[prop % 3], at, around * 1.7F, glm::vec3(1.0F), true);
    }

    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(0.0F));
    orbit.zoom(6.0F);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(glm::vec2(0.1F, 100.0F));
    camera.profile().pixelAspect(static_cast<float>(frameWidth) / static_cast<float>(frameHeight));
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();

    // the default settings, and three metres of margin for a figure's height and its shadow
    const lighting::Settings settings;
    const std::optional<lighting::ShadowBounds> bounds = lighting::fitShadow(registry, {}, {.margin = 3.0F});
    if (!bounds) {
        BOOST_ERROR("a scene with casters gave no fit");
        return;
    }
    const glm::mat4 light = lighting::shadowLight(settings.light, *bounds);
    VkDescriptorSet sceneSet = lit.scene(lighting::pack(settings, light, 1.0F / static_cast<float>(mapSize)),
        headless.context->textures()->depthTexture(*map));

    Frame frame;
    const boost::shared_ptr<Pass> post = frame.pass("grade");
    post->target(output);
    post->reads(scene);
    grade.submit(graded, post.get());

    const boost::shared_ptr<Pass> drawn = frame.pass("lit");
    drawn->target(scene);
    drawn->depth(true);
    drawn->reads(map);
    drawn->clearColour(glm::vec4(0.25F, 0.3F, 0.35F, 1.0F));
    drawn->camera(camera.view(), camera.projection());
    drawn->scene(sceneSet);
    v3d::render::realtime::entity::meshes(registry, 1.0F, meshes, &lit, settings.outline, drawn.get());

    const boost::shared_ptr<Pass> casting = frame.pass("shadow");
    casting->target(map);
    casting->depth(true);
    casting->scene(sceneSet);
    casting->depthBias(settings.constantBias, settings.slopeBias);
    v3d::render::realtime::entity::casters(registry, 1.0F, meshes, &lit, casting.get());

    const std::vector<boost::shared_ptr<Pass>> ordered = frame.ordered();
    BOOST_REQUIRE_EQUAL(ordered.size(), 3U);
    BOOST_CHECK_EQUAL(ordered[0]->name(), "shadow");
    BOOST_CHECK_EQUAL(ordered[1]->name(), "lit");
    BOOST_CHECK_EQUAL(ordered[2]->name(), "grade");

    const boost::shared_ptr<v3d::image::Image> picture = headless.picture(frame, *output, "data_out/lit_scene.png");
    grade.release(graded);
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());

    // something was drawn: the clear colour is not the whole frame, and neither is any one colour
    const unsigned char* first = picture->data();
    std::size_t differing = 0;
    for (std::size_t texel = 0; texel < static_cast<std::size_t>(frameWidth) * frameHeight; texel++) {
        const unsigned char* at = picture->data() + (texel * 4);
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
    const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(*headless.context,
        RenderTarget::Spec{.width = width, .height = height, .colour = colourFormat, .depth = true});

    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless.logger);

    v3d::asset::media::registerLoaders(*assets, headless.logger);
    MeshRegistry meshes(headless.logger, headless.context, assets);
    const MeshHandle pair = meshes.add("pair", twoCubes());

    Lit lit(*headless.context, Lit::Spec{.colour = colourFormat, .depth = target->depthFormat(), .shadow = VK_FORMAT_UNDEFINED});

    entt::registry registry;
    place(&registry, pair, glm::vec3(0.0F), glm::vec3(1.0F), false);

    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(0.0F));
    orbit.zoom(2.5F);
    orbit.elevation(1.0471976F);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(glm::vec2(0.1F, 100.0F));
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();

    lighting::Settings settings;
    settings.outline = 0.0F;

    Frame frame;
    const boost::shared_ptr<Pass> pass = frame.pass("lit");
    pass->target(target);
    pass->depth(true);
    pass->clearColour(glm::vec4(0.0F, 0.0F, 1.0F, 1.0F));
    pass->camera(camera.view(), camera.projection());
    pass->scene(lit.scene(lighting::pack(settings, glm::mat4(1.0F), 0.0F)));
    v3d::render::realtime::entity::meshes(registry, 1.0F, meshes, &lit, settings.outline, pass.get());

    const boost::shared_ptr<v3d::image::Image> picture = headless.picture(frame, *target, "data_out/lit_parts.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());

    // the lit band of each part's own colour, which is pure in its channel
    std::size_t red = 0;
    std::size_t green = 0;
    for (std::size_t texel = 0; texel < static_cast<std::size_t>(width) * height; texel++) {
        const unsigned char* at = picture->data() + (texel * 4);
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
    const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(*headless.context,
        RenderTarget::Spec{.width = width, .height = height, .colour = colourFormat, .depth = true});

    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless.logger);

    v3d::asset::media::registerLoaders(*assets, headless.logger);
    MeshRegistry meshes(headless.logger, headless.context, assets);
    const MeshHandle crate = meshes.add("crate", cube(glm::vec4(1.0F, 0.0F, 0.0F, 1.0F)));

    Lit lit(*headless.context, Lit::Spec{.colour = colourFormat, .depth = target->depthFormat(), .shadow = VK_FORMAT_UNDEFINED});
    World world(*headless.context, World::Spec{.colour = colourFormat, .depth = target->depthFormat()});

    entt::registry registry;
    place(&registry, crate, glm::vec3(0.0F), glm::vec3(1.0F), false);

    // the first case's camera, whose centre ray meets the cube's top face
    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(0.0F));
    orbit.zoom(1.5F);
    orbit.elevation(1.0471976F);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(glm::vec2(0.1F, 100.0F));
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();

    lighting::Settings settings;
    settings.outline = 0.0F;

    v3d::render::realtime::canvas::World ground;
    ground.quad({glm::vec3(-8.0F, -1.0F, -8.0F), glm::vec3(8.0F, -1.0F, -8.0F), glm::vec3(8.0F, -1.0F, 8.0F),
        glm::vec3(-8.0F, -1.0F, 8.0F)}, glm::vec4(0.0F, 1.0F, 0.0F, 1.0F));

    Frame frame;
    const boost::shared_ptr<Pass> pass = frame.pass("lit");
    pass->target(target);
    pass->depth(true);
    pass->clearColour(glm::vec4(0.0F, 0.0F, 1.0F, 1.0F));
    pass->camera(camera.view(), camera.projection());
    pass->scene(lit.scene(lighting::pack(settings, glm::mat4(1.0F), 0.0F)));
    v3d::render::realtime::entity::meshes(registry, 1.0F, meshes, &lit, settings.outline, pass.get());
    world.submit(ground, pass.get());

    const boost::shared_ptr<v3d::image::Image> picture = headless.picture(frame, *target, "data_out/lit_world_quads.png");

    BOOST_CHECK(headless.silent());
    BOOST_REQUIRE(picture);
    const unsigned char* centre = picture->data() + (((static_cast<std::size_t>(height / 2) * width) + (width / 2)) * 4);
    BOOST_TEST_MESSAGE("centre " << int(centre[0]) << "," << int(centre[1]) << "," << int(centre[2]));
    BOOST_CHECK_GT(centre[0], 200);
    BOOST_CHECK_LT(centre[1], 10);

    const unsigned char* corner = picture->data() + (((static_cast<std::size_t>(4) * width) + 4) * 4);
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
    const MeshHandle crate = meshes.add("crate", cube(glm::vec4(1.0F)));

    entt::registry registry;
    place(&registry, crate, glm::vec3(0.0F), glm::vec3(1.0F), false);

    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(0.0F));
    orbit.zoom(1.5F);
    orbit.elevation(1.0471976F);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(glm::vec2(0.1F, 100.0F));
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();

    for (const glm::vec3& colour : {glm::vec3(1.0F), glm::vec3(1.0F, 0.0F, 0.0F)}) {
        const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(*headless.context,
            RenderTarget::Spec{.width = width, .height = height, .colour = colourFormat, .depth = true});
        Lit lit(*headless.context,
            Lit::Spec{.colour = colourFormat, .depth = target->depthFormat(), .shadow = VK_FORMAT_UNDEFINED});

        lighting::Settings settings;
        settings.outline = 0.0F;
        settings.colour = colour;

        Frame frame;
        const boost::shared_ptr<Pass> pass = frame.pass("lit");
        pass->target(target);
        pass->depth(true);
        pass->clearColour(glm::vec4(0.0F, 0.0F, 1.0F, 1.0F));
        pass->camera(camera.view(), camera.projection());
        pass->scene(lit.scene(lighting::pack(settings, glm::mat4(1.0F), 0.0F)));
        v3d::render::realtime::entity::meshes(registry, 1.0F, meshes, &lit, settings.outline, pass.get());

        const boost::shared_ptr<v3d::image::Image> picture = headless.picture(frame, *target, "data_out/lit_coloured.png");
        BOOST_REQUIRE(picture);

        const unsigned char* centre = picture->data() + (((static_cast<std::size_t>(height / 2) * width) + (width / 2)) * 4);
        BOOST_TEST_MESSAGE("centre " << int(centre[0]) << "," << int(centre[1]) << "," << int(centre[2]));
        BOOST_CHECK_EQUAL(static_cast<int>(centre[0]), 255);
        BOOST_CHECK_EQUAL(static_cast<int>(centre[1]), colour.g > 0.0F ? 255 : 0);
        BOOST_CHECK_EQUAL(static_cast<int>(centre[2]), colour.b > 0.0F ? 255 : 0);
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
    const MeshHandle crate = meshes.add("crate", cube(glm::vec4(1.0F, 0.0F, 0.0F, 1.0F)));
    const MeshHandle ground = meshes.add("ground", cube(glm::vec4(1.0F)));

    entt::registry registry;
    place(&registry, crate, glm::vec3(0.0F, 0.5F, 0.0F), glm::vec3(1.0F), false);
    place(&registry, ground, glm::vec3(0.0F, -0.05F, 0.0F), glm::vec3(6.0F, 0.1F, 6.0F), false);

    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(0.0F));
    orbit.zoom(3.0F);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(glm::vec2(0.1F, 100.0F));
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();

    lighting::Settings settings;
    settings.outline = 0.0F;
    settings.colour = glm::vec3(0.45F, 0.55F, 0.9F);
    settings.shadowColour = glm::vec3(0.3F, 0.3F, 0.6F);

    const entt::entity shower = registry.create();
    v3d::ecs::component::Emitter& rain = registry.emplace<v3d::ecs::component::Emitter>(shower, 21U);
    rain.description.direction = glm::vec3(0.0F, -1.0F, 0.0F);
    rain.description.speedMin = 7.0F;
    rain.description.speedMax = 9.0F;
    rain.description.lifeMin = 10.0F;
    rain.description.lifeMax = 10.0F;
    rain.description.cap = 4000;
    rain.description.size = v3d::type::animation::Track<float>(0.03F);
    rain.description.colour = v3d::type::animation::Track<glm::vec4>(glm::vec4(0.6F, 0.7F, 0.9F, 0.5F));
    v3d::render::realtime::component::Particles look;
    look.facing = v3d::render::realtime::component::Particles::Facing::Velocity;
    look.stretch = 0.04F;
    registry.emplace<v3d::render::realtime::component::Particles>(shower, look);

    v3d::type::effect::Weather weather;
    weather.density = 6.0F;
    weather.intensity = 1.0F;
    weather.target = 1.0F;
    weather.wind = glm::vec3(1.5F, 0.0F, 0.0F);

    std::vector<boost::shared_ptr<v3d::image::Image>> frames;
    for (int shot = 0; shot < 3; shot++) {
        for (int stepped = 0; stepped < 30; stepped++) {
            v3d::type::effect::fall(rain.description, &weather, &rain.state, glm::vec3(-3.0F, 0.0F, -3.0F),
                glm::vec3(3.0F, 6.0F, 3.0F), 1.0F / 60.0F);
        }

        const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(*headless.context,
            RenderTarget::Spec{.width = width, .height = height, .colour = colourFormat, .depth = true});
        Lit lit(*headless.context,
            Lit::Spec{.colour = colourFormat, .depth = target->depthFormat(), .shadow = VK_FORMAT_UNDEFINED});
        World world(*headless.context, World::Spec{.colour = colourFormat, .depth = target->depthFormat()});

        v3d::render::realtime::canvas::DepthOrder order;
        const glm::vec3 forward = camera.profile().direction();
        v3d::render::realtime::entity::particles(registry, 1.0F, {.right = camera.profile().right(), .up = camera.profile().up(), .depthAxis = forward}, &order);
        v3d::render::realtime::canvas::World drops;
        order.into(&drops);

        Frame frame;
        const boost::shared_ptr<Pass> pass = frame.pass("lit");
        pass->target(target);
        pass->depth(true);
        pass->clearColour(glm::vec4(0.02F, 0.02F, 0.05F, 1.0F));
        pass->camera(camera.view(), camera.projection());
        pass->scene(lit.scene(lighting::pack(settings, glm::mat4(1.0F), 0.0F)));
        v3d::render::realtime::entity::meshes(registry, 1.0F, meshes, &lit, settings.outline, pass.get());
        world.submit(drops, pass.get(), 0, World::Blend::Additive);

        frames.push_back(headless.picture(frame, *target, "data_out/rain_" + std::to_string(shot) + ".png"));
        BOOST_REQUIRE(frames.back());
    }
    BOOST_CHECK(headless.silent());
    BOOST_CHECK(!v3d::image::compare(*frames[0], *frames[1], 0).match);
    BOOST_CHECK(!v3d::image::compare(*frames[1], *frames[2], 0).match);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

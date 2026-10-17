/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/media/Loaders.h>
#include <api/ecs/component/Playback.h>
#include <api/ecs/component/Transform.h>
#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/MeshRegistry.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/component/Mesh.h>
#include <api/render/realtime/entity/Meshes.h>
#include <api/render/realtime/entity/Poses.h>
#include <api/render/realtime/lighting/SceneUniforms.h>
#include <api/render/realtime/lighting/Settings.h>
#include <api/render/realtime/lighting/Shadow.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/DepthBuffer.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/renderer/Lit.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Isometric.h>

#include <cstddef>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <entt/entt.hpp>
#include <glm/gtc/constants.hpp>

#include "Headless.h"
#include "Shapes.h"

using v3d::ecs::component::Playback;
using v3d::ecs::component::Transform;
using v3d::render::realtime::Frame;
using v3d::render::realtime::MeshHandle;
using v3d::render::realtime::MeshRegistry;
using v3d::render::realtime::Pass;
using v3d::render::realtime::entity::Poses;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::vulkan::frame::Recorder;
using v3d::render::realtime::vulkan::frame::RenderTarget;
using v3d::render::realtime::vulkan::renderer::Lit;

namespace lighting = v3d::render::realtime::lighting;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_SRGB;
const uint32_t width = 256;
const uint32_t height = 256;

// a strip bound to three joints, two units wide and three long, with a bend among its clips
const char* STRIP = "bending_strip.glb";

boost::shared_ptr<v3d::asset::Manager> assets(const v3d::test::Headless& headless) {
    boost::shared_ptr<v3d::asset::Manager> manager =
        boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless.logger);
    v3d::asset::media::registerLoaders(*manager, headless.logger);
    return manager;
}

entt::entity stand(entt::registry* registry, const MeshHandle& mesh, const glm::vec3& position, const glm::vec3& scale,
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
 * A strip laid on its back at a position on x, a quarter turn about x taking its length along
 * -z, and part way through its bend. The bend swings the strip's far end towards -x.
 **/
void lay(entt::registry* registry, const MeshHandle& strip, float x, uint32_t bend, float duration, float through) {
    const entt::entity entity = registry->create();
    Transform transform;
    transform.position = glm::vec3(x, 0.0f, 0.0f);
    transform.rotation = glm::angleAxis(-glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
    registry->emplace<Transform>(entity, transform);
    registry->emplace<v3d::render::realtime::component::Mesh>(entity, v3d::render::realtime::component::Mesh{strip, false});
    Playback playback;
    v3d::ecs::component::play(&playback, bend, duration, false, 0.0f);
    v3d::ecs::component::advance(&playback, duration * through);
    registry->emplace<Playback>(entity, playback);
}

v3d::type::camera::Camera camera(const glm::vec3& target, float zoom) {
    v3d::type::camera::Isometric orbit;
    orbit.target(target);
    orbit.zoom(zoom);
    orbit.elevation(1.0471976f);
    v3d::type::camera::Camera camera;
    camera.profile().clipping(0.1f, 100.0f);
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();
    return camera;
}

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
 * The pixel a point in the world lands on.
 **/
const unsigned char* at(v3d::image::Image& picture, const v3d::type::camera::Camera& camera, const glm::vec3& point) {
    const glm::vec4 clip = camera.projection() * camera.view() * glm::vec4(point, 1.0f);
    const glm::vec2 ndc = glm::vec2(clip) / clip.w;
    const int x = static_cast<int>((ndc.x * 0.5f + 0.5f) * static_cast<float>(picture.width()));
    const int y = static_cast<int>((ndc.y * 0.5f + 0.5f) * static_cast<float>(picture.height()));
    BOOST_REQUIRE(x >= 0 && x < static_cast<int>(picture.width()) && y >= 0 && y < static_cast<int>(picture.height()));
    return picture.data() + (static_cast<std::size_t>(y) * picture.width() + static_cast<std::size_t>(x)) * 4;
}

/**
 * Draw a registry's strips from above, posed and outlined, with no shadow, and read it back.
 **/
boost::shared_ptr<v3d::image::Image> drawStrips(v3d::test::Headless* headless, const entt::registry& registry,
    const MeshRegistry& meshes, Lit* lit, const boost::shared_ptr<RenderTarget>& target, const std::string& path) {
    const v3d::type::camera::Camera view = camera(glm::vec3(0.0f, 0.0f, -1.5f), 6.0f);
    lighting::Settings settings;
    settings.outline = 0.05f;

    const Poses poses = v3d::render::realtime::entity::poses(registry, 1.0f, meshes);
    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("lit");
    pass->target(target);
    pass->depth(true);
    pass->clearColour(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    pass->camera(view.view(), view.projection());
    pass->scene(lit->scene(lighting::pack(settings, glm::mat4(1.0f), 0.0f),
        v3d::render::realtime::TextureHandle(), poses.palette()));
    v3d::render::realtime::entity::meshes(registry, 1.0f, meshes, lit, settings.outline, pass.get(), poses);

    VkCommandBuffer commands = headless->context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless->context->resources(),
        headless->context->frameUniforms().get());
    return readBack(headless, commands, target, path);
}

bool clear(const unsigned char* texel) {
    return texel[0] == 0 && texel[1] == 0 && texel[2] == 255;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(lit_crowd_test)

/**
 * Six entities sharing one model of two parts stand in a grid on the ground, outlined and
 * casting shadows. The top of each part shows that part's own colour at the place its entity
 * stands. A draw that took another entity's transform would put two parts in one place and
 * leave a part's place showing the ground.
 *
 * The picture is written to data_out/lit_crowd.png for a person to look at.
 *
 * Covers the base: 425cda21 brought meshes() and casters(). Checked by giving every item the
 * first entity's model matrix in Meshes.cpp, which fails it.
 **/
BOOST_AUTO_TEST_CASE(every_entity_sharing_a_mesh_is_drawn_where_it_stands) {
    const uint32_t mapSize = 1024;
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, colourFormat, true);
    boost::shared_ptr<RenderTarget> map = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        mapSize, mapSize, VK_FORMAT_UNDEFINED, true, true);

    MeshRegistry meshes(headless.logger, headless.context, assets(headless));
    const MeshHandle pair = meshes.add("pair", v3d::test::twoCubes());
    const MeshHandle ground = meshes.add("ground", v3d::test::cube(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)));
    Lit lit(headless.device, headless.context->pipelineCache(), headless.context->resources(), headless.context->ring(),
        headless.context->frameUniforms(), headless.context->textures(), colourFormat, target->depthFormat(), map->depthFormat());

    entt::registry registry;
    stand(&registry, ground, glm::vec3(0.0f, -0.05f, 0.0f), glm::vec3(12.0f, 0.1f, 8.0f), false);
    std::vector<glm::vec3> places;
    for (const float z : {-1.5f, 1.5f}) {
        for (const float x : {-3.0f, 0.0f, 3.0f}) {
            places.emplace_back(x, 0.5f, z);
            stand(&registry, pair, places.back(), glm::vec3(1.0f), true);
        }
    }

    const v3d::type::camera::Camera view = camera(glm::vec3(0.0f), 6.0f);
    lighting::Settings settings;
    settings.outline = 0.05f;
    const std::optional<lighting::ShadowBounds> bounds = lighting::fitShadow(registry, {}, 2.0f);
    if (!bounds) {
        BOOST_ERROR("a crowd that casts gave no fit");
        return;
    }
    const glm::mat4 light = lighting::shadowLight(settings.light, bounds->centre, bounds->radius);
    VkDescriptorSet scene = lit.scene(lighting::pack(settings, light, 1.0f / static_cast<float>(mapSize)),
        headless.context->textures()->depthTexture(*map));

    Frame frame;
    boost::shared_ptr<Pass> drawn = frame.pass("lit");
    boost::shared_ptr<Pass> casting = frame.pass("shadow");
    casting->target(map);
    casting->depth(true);
    casting->scene(scene);
    casting->depthBias(settings.constantBias, settings.slopeBias);
    v3d::render::realtime::entity::casters(registry, 1.0f, meshes, &lit, casting.get());

    drawn->reads(map);
    drawn->target(target);
    drawn->depth(true);
    drawn->clearColour(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    drawn->camera(view.view(), view.projection());
    drawn->scene(scene);
    v3d::render::realtime::entity::meshes(registry, 1.0f, meshes, &lit, settings.outline, drawn.get());

    VkCommandBuffer commands = headless.context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless.context->resources(),
        headless.context->frameUniforms().get());
    const boost::shared_ptr<v3d::image::Image> picture = readBack(&headless, commands, target, "data_out/lit_crowd.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());

    // the top of each cube faces the light and nothing stands over it, so it is in the lit band
    // of its part's colour, which is pure in its channel
    for (const glm::vec3& place : places) {
        const unsigned char* red = at(*picture, view, place + glm::vec3(-0.75f, 0.5f, 0.0f));
        const unsigned char* green = at(*picture, view, place + glm::vec3(0.75f, 0.5f, 0.0f));
        BOOST_TEST_MESSAGE("at " << place.x << "," << place.z << " red " << int(red[0]) << "," << int(red[1]) << ","
            << int(red[2]) << " green " << int(green[0]) << "," << int(green[1]) << "," << int(green[2]));
        BOOST_CHECK(red[0] > 200 && red[1] < 10 && red[2] < 10);
        BOOST_CHECK(green[1] > 200 && green[0] < 10 && green[2] < 10);
    }
}

/**
 * Two entities sharing one skinned mesh, each at its own point in the bend, draw what each draws
 * alone. Where the left strip alone covers a pixel, the pair shows the left strip's pixel, and
 * everywhere else the right strip's. A draw that read another entity's palette would bend one
 * strip as the other.
 *
 * The pictures are written to data_out/lit_crowd_strips.png, _left and _right.
 *
 * Covers the base: 425cda21 brought poses() and the first joint in each item. Checked by
 * giving every item the first entity's first joint in Meshes.cpp, which fails it.
 **/
BOOST_AUTO_TEST_CASE(each_skinned_entity_sharing_a_mesh_takes_its_own_pose) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, colourFormat, true);
    MeshRegistry meshes(headless.logger, headless.context, assets(headless));
    Lit lit(headless.device, headless.context->pipelineCache(), headless.context->resources(), headless.context->ring(),
        headless.context->frameUniforms(), headless.context->textures(), colourFormat, target->depthFormat(), VK_FORMAT_UNDEFINED);
    const MeshHandle strip = meshes.load(STRIP);
    const std::optional<uint32_t> bend = meshes.clip(strip, "bend");
    BOOST_REQUIRE(bend);
    const float duration = meshes.resolve(strip)->skin->clips[*bend].duration;

    entt::registry both;
    lay(&both, strip, -3.0f, *bend, duration, 0.5f);
    lay(&both, strip, 3.0f, *bend, duration, 0.25f);
    entt::registry left;
    lay(&left, strip, -3.0f, *bend, duration, 0.5f);
    entt::registry right;
    lay(&right, strip, 3.0f, *bend, duration, 0.25f);

    const boost::shared_ptr<v3d::image::Image> pair = drawStrips(&headless, both, meshes, &lit, target, "data_out/lit_crowd_strips.png");
    const boost::shared_ptr<v3d::image::Image> alone = drawStrips(&headless, left, meshes, &lit, target, "data_out/lit_crowd_strips_left.png");
    const boost::shared_ptr<v3d::image::Image> other = drawStrips(&headless, right, meshes, &lit, target, "data_out/lit_crowd_strips_right.png");
    BOOST_REQUIRE(pair && alone && other);
    BOOST_CHECK(headless.silent());

    std::size_t leftCovered = 0;
    std::size_t rightCovered = 0;
    std::size_t differing = 0;
    for (std::size_t texel = 0; texel < static_cast<std::size_t>(width) * height; texel++) {
        const unsigned char* fromLeft = alone->data() + texel * 4;
        const unsigned char* fromRight = other->data() + texel * 4;
        leftCovered += clear(fromLeft) ? 0 : 1;
        rightCovered += clear(fromRight) ? 0 : 1;
        const unsigned char* expected = clear(fromLeft) ? fromRight : fromLeft;
        if (std::memcmp(pair->data() + texel * 4, expected, 4) != 0) {
            differing++;
        }
    }
    BOOST_TEST_MESSAGE("left covers " << leftCovered << ", right covers " << rightCovered << ", differing " << differing);
    BOOST_CHECK_GT(leftCovered, 500U);
    BOOST_CHECK_GT(rightCovered, 500U);
    BOOST_CHECK_EQUAL(differing, 0U);
}

/**
 * Six entities sharing one model of two parts take one draw per part in each walk: two outline
 * draws and two surface draws, each of six instances. The same six, each given its own copy of
 * the model under its own key, take a draw per part per entity. Both draw the same picture, byte
 * for byte, since an instance reads the same object a lone draw would.
 *
 * The pictures are written to data_out/lit_crowd_shared.png and data_out/lit_crowd_apart.png.
 **/
BOOST_AUTO_TEST_CASE(entities_sharing_a_mesh_are_drawn_in_one_draw_per_part) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, colourFormat, true);
    MeshRegistry meshes(headless.logger, headless.context, assets(headless));
    Lit lit(headless.device, headless.context->pipelineCache(), headless.context->resources(), headless.context->ring(),
        headless.context->frameUniforms(), headless.context->textures(), colourFormat, target->depthFormat(), VK_FORMAT_UNDEFINED);

    std::vector<MeshHandle> shared;
    std::vector<MeshHandle> apart;
    for (int copy = 0; copy < 6; copy++) {
        apart.push_back(meshes.add("pair " + std::to_string(copy), v3d::test::twoCubes()));
        shared.push_back(copy == 0 ? meshes.add("pair", v3d::test::twoCubes()) : shared.front());
    }

    const v3d::type::camera::Camera view = camera(glm::vec3(0.0f), 6.0f);
    lighting::Settings settings;
    settings.outline = 0.05f;

    const auto draw = [&](const std::vector<MeshHandle>& handles, const std::string& path, std::size_t* items) {
        entt::registry registry;
        std::size_t next = 0;
        for (const float z : {-1.5f, 1.5f}) {
            for (const float x : {-3.0f, 0.0f, 3.0f}) {
                stand(&registry, handles[next++], glm::vec3(x, 0.5f, z), glm::vec3(1.0f), false);
            }
        }
        Frame frame;
        boost::shared_ptr<Pass> pass = frame.pass("lit");
        pass->target(target);
        pass->depth(true);
        pass->clearColour(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
        pass->camera(view.view(), view.projection());
        pass->scene(lit.scene(lighting::pack(settings, glm::mat4(1.0f), 0.0f)));
        v3d::render::realtime::entity::meshes(registry, 1.0f, meshes, &lit, settings.outline, pass.get());
        *items = pass->items().size();

        VkCommandBuffer commands = headless.context->ring()->begin();
        Recorder::record(commands, frame, Recorder::Target(), *headless.context->resources(),
            headless.context->frameUniforms().get());
        return readBack(&headless, commands, target, path);
    };

    std::size_t sharedItems = 0;
    std::size_t apartItems = 0;
    const boost::shared_ptr<v3d::image::Image> together = draw(shared, "data_out/lit_crowd_shared.png", &sharedItems);
    const boost::shared_ptr<v3d::image::Image> separately = draw(apart, "data_out/lit_crowd_apart.png", &apartItems);
    BOOST_REQUIRE(together && separately);
    BOOST_CHECK(headless.silent());

    BOOST_CHECK_EQUAL(sharedItems, 4U);
    BOOST_CHECK_EQUAL(apartItems, 24U);
    std::size_t covered = 0;
    std::size_t differing = 0;
    for (std::size_t texel = 0; texel < static_cast<std::size_t>(width) * height; texel++) {
        covered += clear(together->data() + texel * 4) ? 0 : 1;
        differing += std::memcmp(together->data() + texel * 4, separately->data() + texel * 4, 4) == 0 ? 0 : 1;
    }
    BOOST_TEST_MESSAGE("covered " << covered << ", differing " << differing);
    BOOST_CHECK_GT(covered, 1000U);
    BOOST_CHECK_EQUAL(differing, 0U);
}

/**
 * A grid of 576 cubes, each one object, is more than the scene's object buffer starts with. It
 * is drawn in two frames, the first left in flight while the second is built, and the top of
 * every cube in the second shows red where the cube stands. The buffer grows part way through
 * the walk, and the objects added before that must reach the bigger buffer.
 *
 * The picture is written to data_out/lit_crowd_grown.png.
 *
 * Passes before the change: each draw pushed its own object, so there was no buffer to outgrow.
 **/
BOOST_AUTO_TEST_CASE(a_crowd_larger_than_the_object_buffer_draws_every_object) {
    const uint32_t size = 512;
    const int side = 24;
    v3d::test::Headless headless(colourFormat, size, size);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        size, size, colourFormat, true);
    MeshRegistry meshes(headless.logger, headless.context, assets(headless));
    const MeshHandle crate = meshes.add("crate", v3d::test::cube(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)));
    Lit lit(headless.device, headless.context->pipelineCache(), headless.context->resources(), headless.context->ring(),
        headless.context->frameUniforms(), headless.context->textures(), colourFormat, target->depthFormat(), VK_FORMAT_UNDEFINED);

    entt::registry registry;
    std::vector<glm::vec3> places;
    for (int row = 0; row < side; row++) {
        for (int column = 0; column < side; column++) {
            places.emplace_back(static_cast<float>(column) - 11.5f, 0.3f, static_cast<float>(row) - 11.5f);
            stand(&registry, crate, places.back(), glm::vec3(0.6f), false);
        }
    }

    const v3d::type::camera::Camera view = camera(glm::vec3(0.0f), 13.0f);
    lighting::Settings settings;
    settings.outline = 0.0f;

    boost::shared_ptr<v3d::image::Image> picture;
    for (int drawn = 1; drawn <= 2; drawn++) {
        Frame frame;
        boost::shared_ptr<Pass> pass = frame.pass("lit");
        pass->target(target);
        pass->depth(true);
        pass->clearColour(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
        pass->camera(view.view(), view.projection());
        pass->scene(lit.scene(lighting::pack(settings, glm::mat4(1.0f), 0.0f)));
        v3d::render::realtime::entity::meshes(registry, 1.0f, meshes, &lit, settings.outline, pass.get());
        // every cube shares one mesh of one part, so the walk is one draw of every instance
        BOOST_CHECK_EQUAL(pass->items().size(), 1U);

        VkCommandBuffer commands = headless.context->ring()->begin();
        Recorder::record(commands, frame, Recorder::Target(), *headless.context->resources(),
            headless.context->frameUniforms().get());
        if (drawn == 2) {
            picture = readBack(&headless, commands, target, "data_out/lit_crowd_grown.png");
        } else {
            headless.submit(commands);
        }
    }
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());

    std::size_t red = 0;
    for (const glm::vec3& place : places) {
        const unsigned char* top = at(*picture, view, place + glm::vec3(0.0f, 0.3f, 0.0f));
        if (top[0] > 200 && top[1] < 10 && top[2] < 10) {
            red++;
        }
    }
    BOOST_TEST_MESSAGE("red tops " << red << " of " << places.size());
    BOOST_CHECK_EQUAL(red, places.size());
}

/**
 * An object added before any scene is written is refused, since there is no frame for it to
 * belong to.
 **/
BOOST_AUTO_TEST_CASE(an_object_added_before_any_scene_is_refused) {
    v3d::test::Headless headless(colourFormat, width, height);
    Lit lit(headless.device, headless.context->pipelineCache(), headless.context->resources(), headless.context->ring(),
        headless.context->frameUniforms(), headless.context->textures(), colourFormat,
        v3d::render::realtime::vulkan::frame::DepthBuffer::chooseFormat(headless.device->physical()), VK_FORMAT_UNDEFINED);
    BOOST_CHECK_THROW(lit.object(Lit::Object()), std::runtime_error);
    BOOST_CHECK(headless.silent());
}

BOOST_AUTO_TEST_SUITE_END()

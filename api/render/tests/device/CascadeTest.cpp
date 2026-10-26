/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/media/Loaders.h>
#include <api/ecs/component/Transform.h>
#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/MeshRegistry.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/Textures.h>
#include <api/render/realtime/component/Mesh.h>
#include <api/render/realtime/entity/Meshes.h>
#include <api/render/realtime/lighting/SceneUniforms.h>
#include <api/render/realtime/lighting/Settings.h>
#include <api/render/realtime/lighting/Shadow.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/frame/Timings.h>
#include <api/render/realtime/vulkan/renderer/Lit.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Headless.h"
#include "Shapes.h"

using v3d::ecs::component::Transform;
using v3d::render::realtime::Frame;
using v3d::render::realtime::MeshHandle;
using v3d::render::realtime::MeshRegistry;
using v3d::render::realtime::Pass;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::vulkan::frame::Recorder;
using v3d::render::realtime::vulkan::frame::RenderTarget;
using v3d::render::realtime::vulkan::frame::Timings;
using v3d::render::realtime::vulkan::renderer::Lit;
using v3d::test::cube;

namespace lighting = v3d::render::realtime::lighting;

namespace {

// the lit pipelines compute in linear light, and an SRGB target encodes it on store
const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_SRGB;
const uint32_t size = 256;
const uint32_t mapSize = 512;

void place(entt::registry* registry, const MeshHandle& mesh, const glm::vec3& position, const glm::vec3& scale, bool castsShadow) {
    const entt::entity entity = registry->create();
    Transform transform;
    transform.position = position;
    transform.scale = scale;
    registry->emplace<Transform>(entity, transform);
    registry->emplace<v3d::render::realtime::component::Mesh>(entity, v3d::render::realtime::component::Mesh{mesh, castsShadow});
}

/** A cube standing on the ground, the near, middle and far one each in a cascade of its own. **/
struct Standing final {
    glm::vec3 position;
    float scale;
};

std::array<Standing, 3> standing() {
    return {{
        {.position = glm::vec3(0.0F, 0.5F, -7.0F), .scale = 1.0F},
        {.position = glm::vec3(-3.0F, 1.0F, -22.0F), .scale = 2.0F},
        {.position = glm::vec3(4.0F, 2.0F, -60.0F), .scale = 4.0F},
    }};
}

/** Where each cascade ends, from the camera's near plane. **/
const std::array<float, 3> splits = {18.0F, 40.0F, 100.0F};

/** What a frame of the long view drew, and how. **/
struct Drawn final {
    boost::shared_ptr<v3d::image::Image> picture;
    /** how many casters each cascade's pass drew **/
    std::vector<uint32_t> casters;
    /** whether a caster's depth landed anywhere in each cascade's layer **/
    std::vector<bool> layered;
    std::vector<Timings::Timing> spans;
};

/**
 * A long white ground and three red cubes down it, seen from above one end, after a shadow pass
 * into each of three cascades. The light is behind the cubes, so each shadow falls towards the
 * camera, where it can be seen.
 *
 * @param strength lighting::Settings::shadowStrength, so a case can draw the same frame with the
 *        shadow ignored
 **/
Drawn drawLongView(v3d::test::Headless* headless, float strength, const std::string& path, const v3d::type::camera::Camera& camera) {
    Drawn drawn;
    const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless->device, headless->context->ring(),
        size, size, colourFormat, true);
    const boost::shared_ptr<RenderTarget> map = boost::make_shared<RenderTarget>(headless->device, headless->context->ring(),
        mapSize, mapSize, VK_FORMAT_UNDEFINED, true, true, 1, static_cast<uint32_t>(splits.size()));

    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(V3D_ASSET_FIXTURES, headless->logger);
    v3d::asset::media::registerLoaders(*assets, headless->logger);
    MeshRegistry meshes(headless->logger, headless->context, assets);
    const MeshHandle crate = meshes.add("crate", cube(glm::vec4(1.0F, 0.0F, 0.0F, 1.0F)));
    const MeshHandle ground = meshes.add("ground", cube(glm::vec4(1.0F)));
    Lit lit(headless->device, headless->context->pipelineCache(), headless->context->resources(), headless->context->ring(),
        headless->context->frameUniforms(), headless->context->textures(), colourFormat, target->depthFormat(), map->depthFormat());

    entt::registry registry;
    for (const Standing& cube : standing()) {
        place(&registry, crate, cube.position, glm::vec3(cube.scale), true);
    }
    // thin, topped at zero, and casting nothing
    place(&registry, ground, glm::vec3(0.0F, -0.05F, -55.0F), glm::vec3(30.0F, 0.1F, 120.0F), false);

    lighting::Settings settings;
    settings.outline = 0.0F;
    settings.shadowStrength = strength;
    settings.light = glm::vec3(0.5F, 0.7F, -0.5F);

    const std::optional<lighting::Cascades> cascades =
        lighting::fitCascades(camera.projection() * camera.view(), splits, settings.light, {.texels = mapSize});
    if (!cascades) {
        BOOST_ERROR("the long view gave no cascades");
        return drawn;
    }
    VkDescriptorSet scene = lit.scene(lighting::pack(settings, *cascades, 1.0F / mapSize),
        headless->context->textures()->depthTexture(*map));

    Frame frame;
    const boost::shared_ptr<Pass> pass = frame.pass("lit");
    for (uint32_t cascade = 0; cascade < cascades->count; cascade++) {
        const boost::shared_ptr<Pass> casting = frame.pass("shadow " + std::to_string(cascade));
        casting->target(map);
        casting->layer(cascade);
        casting->depth(true);
        casting->scene(scene);
        casting->depthBias(settings.constantBias, settings.slopeBias);
        v3d::render::realtime::entity::casters(registry, 1.0F, meshes, &lit, casting.get(), {},
            {.cascade = cascade, .covers = cascades->bounds[cascade], .towards = settings.light});
        uint32_t drawnHere = 0;
        for (const v3d::render::realtime::DrawItem& item : casting->items()) {
            drawnHere += item.instances;
        }
        drawn.casters.push_back(drawnHere);
    }
    pass->reads(map);
    pass->target(target);
    pass->depth(true);
    pass->clearColour(glm::vec4(0.0F, 0.0F, 1.0F, 1.0F));
    pass->camera(camera.view(), camera.projection());
    pass->scene(scene);
    v3d::render::realtime::entity::meshes(registry, 1.0F, meshes, &lit, settings.outline, pass.get());

    // a slot's spans are read when the slot is begun again, so one full round of the ring is
    // recorded before the frame that is kept
    Timings& timings = headless->context->ring()->timings();
    const uint32_t frames = headless->context->ring()->framesInFlight() + 1;
    for (uint32_t index = 0; index + 1 < frames; ++index) {
        VkCommandBuffer commands = headless->context->ring()->begin();
        Recorder::record(commands, frame, Recorder::Target(), *headless->context->resources(),
            headless->context->frameUniforms().get(), &timings);
        headless->submitAndWait(commands);
    }
    VkCommandBuffer commands = headless->context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless->context->resources(),
        headless->context->frameUniforms().get(), &timings);

    Capture capture(headless->device, headless->context->ring(), headless->logger);
    Capture::Source source;
    source.image = target->image();
    source.extent = target->extent();
    source.format = target->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture.record(commands, source);
    headless->submitAndWait(commands);
    drawn.spans = timings.last();
    if (capture.write(path)) {
        v3d::image::reader::Png png(headless->logger);
        drawn.picture = png.read(path);
    }

    // each layer read back on its own, for a caster's depth anywhere in it
    for (uint32_t cascade = 0; map->depthFormat() == VK_FORMAT_D32_SFLOAT && cascade < cascades->count; cascade++) {
        Capture layer(headless->device, headless->context->ring(), headless->logger);
        Capture::Source depth;
        depth.image = map->depthImage();
        depth.extent = map->extent();
        depth.format = map->depthFormat();
        depth.layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
        depth.depth = true;
        depth.layer = cascade;
        VkCommandBuffer reading = headless->context->ring()->begin();
        layer.record(reading, depth);
        headless->submitAndWait(reading);
        const std::vector<float> depths = layer.depth();
        drawn.layered.push_back(std::ranges::any_of(depths, [](float stored) { return stored < 1.0F; }));
    }
    return drawn;
}

/**
 * A camera above the near end of the ground, looking down its length.
 **/
v3d::type::camera::Camera longView() {
    v3d::type::camera::Camera camera;
    v3d::type::camera::Profile& profile = camera.profile();
    profile.orthographic(false);
    profile.fov(60.0F);
    profile.pixelAspect(1.0F);
    profile.clipping(glm::vec2(0.5F, 150.0F));
    profile.eye(glm::vec3(0.0F, 12.0F, 6.0F));
    profile.up(glm::vec3(0.0F, 1.0F, 0.0F));
    profile.lookat(glm::vec3(0.0F, 0.0F, -30.0F));
    camera.createProjection();
    camera.createView();
    return camera;
}

/** The same frame drawn with its shadow ignored and with it. **/
struct Pictures final {
    boost::shared_ptr<v3d::image::Image> without;
    boost::shared_ptr<v3d::image::Image> with;
};

/**
 * Whether the ground went from white to grey anywhere within a few pixels of a point.
 **/
bool darkenedNear(const Pictures& pictures, const glm::ivec2& at) {
    const boost::shared_ptr<v3d::image::Image>& without = pictures.without;
    const boost::shared_ptr<v3d::image::Image>& with = pictures.with;
    const int reach = 6;
    for (int y = std::max(at.y - reach, 0); y <= std::min(at.y + reach, static_cast<int>(with->height()) - 1); y++) {
        for (int x = std::max(at.x - reach, 0); x <= std::min(at.x + reach, static_cast<int>(with->width()) - 1); x++) {
            const std::size_t texel = (static_cast<std::size_t>(y) * with->width()) + static_cast<std::size_t>(x);
            const unsigned char* before = without->data() + (texel * 4);
            const unsigned char* after = with->data() + (texel * 4);
            const bool white = before[0] == 255 && before[1] == 255 && before[2] == 255;
            const bool grey = after[0] == after[1] && after[1] == after[2] && after[0] < 255;
            if (white && grey) {
                return true;
            }
        }
    }
    return false;
}

BOOST_AUTO_TEST_SUITE(cascade_test)

/**
 * A long view drawn with three cascades. Each cascade's pass draws its casters into its own
 * layer, and the near one leaves out the casters that cannot reach it. On the ground, each
 * cube's shadow falls where its own cascade puts it, which only a map of three layers read
 * through the right matrix gives. The validation layer reports nothing, and the pass timings
 * hold each cascade's pass under its name, which is what each one costs.
 *
 * The pictures are written to data_out/cascades.png and data_out/cascades_unshadowed.png for a
 * person to look at.
 **/
BOOST_AUTO_TEST_CASE(a_long_view_is_shadowed_by_three_cascades) {
    v3d::test::Headless headless(colourFormat, size, size);
    const v3d::type::camera::Camera camera = longView();
    const Drawn ignored = drawLongView(&headless, 0.0F, "data_out/cascades_unshadowed.png", camera);
    const Drawn shadowed = drawLongView(&headless, 1.0F, "data_out/cascades.png", camera);
    BOOST_REQUIRE(ignored.picture);
    BOOST_REQUIRE(shadowed.picture);
    BOOST_CHECK(headless.silent());

    // the near cascade is the smallest, and cannot reach the far cube
    BOOST_REQUIRE_EQUAL(shadowed.casters.size(), splits.size());
    BOOST_CHECK_GE(shadowed.casters.front(), 1U);
    BOOST_CHECK_LT(shadowed.casters.front(), static_cast<uint32_t>(standing().size()));
    for (std::size_t cascade = 0; cascade < shadowed.layered.size(); cascade++) {
        BOOST_TEST_CONTEXT("cascade " << cascade) {
            BOOST_CHECK(shadowed.layered[cascade]);
        }
    }

    // each cube's shadow falls along the light from its middle to the ground
    const glm::vec3 towards = glm::normalize(glm::vec3(0.5F, 0.7F, -0.5F));
    const glm::mat4 viewProjection = camera.projection() * camera.view();
    const std::array<Standing, 3> cubes = standing();
    for (std::size_t index = 0; index < cubes.size(); index++) {
        const glm::vec3& middle = cubes[index].position;
        const glm::vec3 ground = middle - towards * (middle.y / towards.y);
        const glm::vec4 clip = viewProjection * glm::vec4(ground, 1.0F);
        const glm::vec2 ndc = glm::vec2(clip) / clip.w;
        const glm::ivec2 pixel(((ndc.x * 0.5F) + 0.5F) * static_cast<float>(size), ((ndc.y * 0.5F) + 0.5F) * static_cast<float>(size));
        BOOST_TEST_CONTEXT("cube " << index << " at pixel " << pixel.x << ", " << pixel.y) {
            BOOST_CHECK(darkenedNear({.without = ignored.picture, .with = shadowed.picture}, pixel));
        }
    }

    if (!headless.context->ring()->timings().enabled()) {
        BOOST_TEST_MESSAGE("the graphics queue writes no timestamps, so there is nothing to time");
        return;
    }
    std::vector<std::string> names;
    for (const Timings::Timing& span : shadowed.spans) {
        names.push_back(span.name);
        BOOST_TEST_MESSAGE(span.name << " took " << span.milliseconds << " ms");
    }
    for (uint32_t cascade = 0; cascade < splits.size(); cascade++) {
        BOOST_CHECK(std::ranges::find(names, "shadow " + std::to_string(cascade)) != names.end());
    }
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

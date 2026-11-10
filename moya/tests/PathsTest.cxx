/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Compare.h>
#include <moya/libmoya/film/Estimate.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/kernel/integrate/Settings.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <cstdint>
#include <string>

#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace colour = v3d::moya::colour;
namespace film = v3d::moya::film;
namespace integrate = v3d::moya::integrate;
namespace scene = v3d::moya::scene;
typedef v3d::moya::device::CpuDevice CpuDevice;

/** The side of each picture, in pixels. **/
constexpr std::uint32_t kSide = 32;

/** The pixels well inside the sphere's picture, and those well outside it in a corner. **/
const glm::uvec2 kInsideFrom { 12, 12 };
const glm::uvec2 kInsideTo { 20, 20 };
const glm::uvec2 kOutsideFrom { 0, 0 };
const glm::uvec2 kOutsideTo { 4, 4 };

/**
 * A scene of a camera five units back along -z looking at the origin, at kSide pixels square and
 * a field of view of thirty degrees, with samples per pixel and a white environment.
 **/
scene::Handle framed(scene::Scene & scene, std::uint32_t samples) {
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(options, "resolution", scene::Value::integers({ kSide, kSide }));
    scene.set(options, "samples", scene::Value::integer(static_cast<std::int32_t>(samples)));
    scene.set(options, "environment", scene::Value::colour(glm::vec3 { 1.0F }));
    scene.set(camera, "fov", scene::Value::number(30.0F));
    scene.set(camera, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -5.0F)));
    return options;
}

/** A sphere of radius one at the origin, of a constant colour. **/
scene::Handle ball(scene::Scene & scene, const std::string & name, float grey, const glm::mat4 & place) {
    const scene::Handle sphere = scene.create("sphere", name);
    scene.declare(sphere, scene::Declaration { .name = "Cs",
        .type = scene::Type::Colour,
        .count = scene::Declaration::kAny,
        .fallback = scene::Value::colours({}),
        .interpolation = scene::Interpolation::Constant });
    scene.set(sphere, "Cs", scene::Value::colours({ glm::vec3 { grey } }));
    return instanced(scene, name, sphere, {}, place);
}

/** A scene of one sphere of a grey in the white environment, with a depth limit. **/
film::Snapshot sphereIn(float grey, std::uint32_t depth, std::uint32_t samples = 64) {
    scene::Scene scene(scene::kinds());
    const scene::Handle options = framed(scene, samples);
    scene.set(options, "maxDepth", scene::Value::integer(static_cast<std::int32_t>(depth)));
    ball(scene, "ball", grey, glm::mat4(1.0F));
    scene.commit();
    scene.apply();
    CpuDevice device;
    return integrate::render(device, scene, {}, colour::Config());
}

/**
 * A grey sphere resting on a grey plane under the white environment, so light bounces between
 * them, with Russian roulette from a depth.
 **/
film::Snapshot sphereOnPlane(std::uint32_t roulette, unsigned int threads = 0) {
    scene::Scene scene(scene::kinds());
    const scene::Handle options = framed(scene, 256);
    scene.set(options, "rouletteDepth", scene::Value::integer(static_cast<std::int32_t>(roulette)));
    scene.set(options, "diffuseDepth", scene::Value::integer(12));
    ball(scene, "ball", 0.7F, glm::mat4(1.0F));
    const scene::Handle floor = scene.create("disk", "floor");
    scene.set(floor, "radius", scene::Value::number(20.0F));
    scene.declare(floor, scene::Declaration { .name = "Cs",
        .type = scene::Type::Colour,
        .count = scene::Declaration::kAny,
        .fallback = scene::Value::colours({}),
        .interpolation = scene::Interpolation::Constant });
    scene.set(floor, "Cs", scene::Value::colours({ glm::vec3 { 0.6F } }));
    // the disk faces +z in its own space, so it is turned to face up, and lies under the sphere
    instanced(scene, "floor", floor, {}, glm::rotate(moved(0.0F, -1.0F, 0.0F), glm::radians(90.0F), glm::vec3 { 1.0F, 0.0F, 0.0F }));
    scene.commit();
    scene.apply();
    CpuDevice device(threads);
    return integrate::render(device, scene, {}, colour::Config());
}

};  // namespace

// a white sphere in a white environment is white wherever it is seen: every path reaches the
// environment carrying all of its light
BOOST_AUTO_TEST_CASE(moya_paths_a_white_sphere_in_a_white_furnace_is_white_test) {
    const film::Snapshot snapshot = sphereIn(1.0F, 12);
    const film::Furthest furthest = film::furthest(snapshot, [](const glm::uvec2 &) { return 1.0F; });
    BOOST_TEST(furthest.sigmas < 5.0F);
    BOOST_TEST(furthest.unmeasured == 0U);
    BOOST_TEST(film::sigmas(film::mean(snapshot, { 0, 0 }, { kSide, kSide }), 1.0F) < 3.0F);
    BOOST_TEST(film::sigmas(film::mean(snapshot, kInsideFrom, kInsideTo), 1.0F) < 3.0F);
}

// a convex grey sphere never meets itself, so it shows its albedo times the environment
BOOST_AUTO_TEST_CASE(moya_paths_a_grey_sphere_shows_its_albedo_test) {
    const film::Snapshot snapshot = sphereIn(0.5F, 12);
    BOOST_TEST(film::sigmas(film::mean(snapshot, kInsideFrom, kInsideTo), 0.5F) < 3.0F);
    BOOST_TEST(film::sigmas(film::mean(snapshot, kOutsideFrom, kOutsideTo), 1.0F) < 3.0F);
    BOOST_TEST(film::sigmas(film::mean(snapshot, kInsideFrom, kInsideTo), 0.6F) > 10.0F);
}

// with no bounce allowed, a camera ray that meets the sphere gathers nothing, and one that misses
// it still meets the environment. A diffuse limit of zero ends the same paths
BOOST_AUTO_TEST_CASE(moya_paths_a_depth_limit_of_zero_shows_the_environment_alone_test) {
    const film::Snapshot snapshot = sphereIn(1.0F, 0);
    BOOST_TEST(film::mean(snapshot, kInsideFrom, kInsideTo).value == 0.0F);
    BOOST_TEST(film::sigmas(film::mean(snapshot, kOutsideFrom, kOutsideTo), 1.0F) < 3.0F);

    scene::Scene scene(scene::kinds());
    const scene::Handle options = framed(scene, 16);
    scene.set(options, "diffuseDepth", scene::Value::integer(0));
    ball(scene, "ball", 1.0F, glm::mat4(1.0F));
    scene.commit();
    scene.apply();
    CpuDevice device;
    BOOST_TEST(film::mean(integrate::render(device, scene, {}, colour::Config()), kInsideFrom, kInsideTo).value == 0.0F);
}

// Russian roulette ends paths early and weighs the survivors up, so a picture with it from the
// first bounce has the mean of one without it before the twelfth
BOOST_AUTO_TEST_CASE(moya_paths_russian_roulette_keeps_the_mean_test) {
    const film::Snapshot early = sphereOnPlane(1);
    const film::Snapshot late = sphereOnPlane(12);
    const film::Estimate earlyMean = film::mean(early, { 0, 0 }, { kSide, kSide });
    const film::Estimate lateMean = film::mean(late, { 0, 0 }, { kSide, kSide });
    BOOST_TEST(film::sigmas(earlyMean, lateMean) < 3.0F);
    BOOST_TEST(film::furthest(early, late).sigmas < 5.0F);
    // the floor under the sphere is shadowed by it, so the picture is no flat grey
    BOOST_TEST(film::mean(late, { 12, 26 }, { 20, 30 }).value < film::mean(late, kOutsideFrom, kOutsideTo).value);
}

BOOST_AUTO_TEST_CASE(moya_paths_a_picture_is_the_same_on_one_thread_and_on_every_thread_test) {
    const film::Snapshot one = sphereOnPlane(3, 1);
    const film::Snapshot every = sphereOnPlane(3, 0);
    BOOST_TEST(one.values == every.values, boost::test_tools::per_element());
}

// the settings are the options', each depth held to zero or more, and the environment converted
// into ACEScg from the space its colour names
BOOST_AUTO_TEST_CASE(moya_paths_the_settings_are_the_options_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle options = scene.create("options", "options");
    scene.set(options, "maxDepth", scene::Value::integer(7));
    scene.set(options, "diffuseDepth", scene::Value::integer(-2));
    scene.set(options, "glossyDepth", scene::Value::integer(3));
    scene.set(options, "specularDepth", scene::Value::integer(5));
    scene.set(options, "transmissionDepth", scene::Value::integer(6));
    scene.set(options, "rouletteDepth", scene::Value::integer(2));
    scene.set(options, "environment", scene::Value::colour(glm::vec3 { 0.8F, 0.3F, 0.1F }, "Linear Rec.709 (sRGB)"));
    scene.commit();
    scene.apply();
    const colour::Config config;
    const v3d::moya::kernel::integrate::Settings settings = integrate::settingsOf(scene, config);
    BOOST_TEST(settings.maxDepth == 7U);
    BOOST_TEST(settings.diffuseDepth == 0U);
    BOOST_TEST(settings.glossyDepth == 3U);
    BOOST_TEST(settings.specularDepth == 5U);
    BOOST_TEST(settings.transmissionDepth == 6U);
    BOOST_TEST(settings.rouletteDepth == 2U);
    BOOST_TEST(glm::distance(settings.environment, config.input("Linear Rec.709 (sRGB)").apply({ 0.8F, 0.3F, 0.1F })) <= 1.0e-6F);

    scene::Scene empty(scene::kinds());
    const v3d::moya::kernel::integrate::Settings fallback = integrate::settingsOf(empty, config);
    BOOST_TEST(fallback.maxDepth == 12U);
    BOOST_TEST(fallback.diffuseDepth == 4U);
    BOOST_TEST(fallback.rouletteDepth == 3U);
    BOOST_TEST(glm::distance(fallback.environment, glm::vec3 { 0.0F }) == 0.0F);
}

};  // namespace v3d::moya::tracing

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Compare.h>
#include <moya/libmoya/film/Estimate.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/integrate/Profile.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/kernel/bsdf/Model.h>
#include <moya/libmoya/kernel/integrate/Settings.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/output/Checkpoint.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
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
namespace geometry = v3d::moya::geometry;
namespace kernel = v3d::moya::kernel;
namespace integrate = v3d::moya::integrate;
namespace output = v3d::moya::output;
namespace scene = v3d::moya::scene;
typedef v3d::moya::device::CpuDevice CpuDevice;

/** The side of each picture, in pixels. **/
constexpr std::uint32_t kSide = 32;

/**
 * The square of pixels well inside the sphere's picture, and the one well outside it in a corner:
 * the first pixel along each axis, and the one past the last.
 **/
constexpr std::uint32_t kInsideFrom = 12;
constexpr std::uint32_t kInsideTo = 20;
constexpr std::uint32_t kOutsideFrom = 0;
constexpr std::uint32_t kOutsideTo = 4;

/** The adaptive sampling threshold of the cases that sample adaptively. **/
constexpr float kThreshold = 0.05F;

/**
 * A scene of a camera five units back along -z looking at the origin, at kSide pixels square and
 * a field of view of thirty degrees, with samples per pixel. A white dome light of no image
 * lights it.
 **/
scene::Handle framed(scene::Scene & scene, std::uint32_t samples) {
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(options, "resolution", scene::Value::integers({ kSide, kSide }));
    scene.set(options, "samples", scene::Value::integer(static_cast<std::int32_t>(samples)));
    scene.create("domeLight", "sky");
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

/** A scene of one sphere of a grey under the white dome, with a depth limit. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the grey, the depth, then the samples
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
 * Builds a sphere of a grey resting on a grey plane under the white dome, so light bounces
 * between them. The options give the samples per pixel, Russian roulette from a depth, and
 * adaptive sampling to a noise threshold. The scene is committed and applied.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the settings, then the grey
void buildSphereOnPlane(scene::Scene & scene, std::uint32_t samples, std::uint32_t roulette, float threshold, float grey = 0.7F) {
    const scene::Handle options = framed(scene, samples);
    scene.set(options, "noiseThreshold", scene::Value::number(threshold));
    scene.set(options, "rouletteDepth", scene::Value::integer(static_cast<std::int32_t>(roulette)));
    scene.set(options, "diffuseDepth", scene::Value::integer(12));
    ball(scene, "ball", grey, glm::mat4(1.0F));
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
}

/** The sphere on the plane at 256 samples per pixel, rendered on a number of threads. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the depth, then the threads
film::Snapshot sphereOnPlane(std::uint32_t roulette, unsigned int threads = 0, float threshold = 0.0F) {
    scene::Scene scene(scene::kinds());
    buildSphereOnPlane(scene, 256, roulette, threshold);
    CpuDevice device(threads);
    return integrate::render(device, scene, {}, colour::Config());
}

/** A checkpoint's path in the temporary directory, with no file there. **/
std::string checkpointNamed(const std::string & name) {
    const boost::filesystem::path path = boost::filesystem::temp_directory_path() / ("v3d_paths_" + name + ".exr");
    boost::system::error_code ignored;
    boost::filesystem::remove(path, ignored);
    return path.string();
}

/**
 * The sphere on the plane at 128 samples per pixel, eight passes, rendered on a number of threads
 * with checkpoints.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the threshold, then the threads
film::Snapshot checkpointed(float threshold, unsigned int threads, const integrate::Checkpoints & checkpoints, float grey = 0.7F) {
    scene::Scene scene(scene::kinds());
    buildSphereOnPlane(scene, 128, 3, threshold, grey);
    CpuDevice device(threads);
    return integrate::render(device, scene, {}, colour::Config(), checkpoints);
}

/** The fingerprint of checkpointed()'s scene at a threshold. **/
std::uint64_t printOf(float threshold) {
    scene::Scene scene(scene::kinds());
    buildSphereOnPlane(scene, 128, 3, threshold);
    return integrate::fingerprint(scene, {});
}

/** An RI surface, matte unless named, of a constant grey Cs, and a material of that surface. **/
scene::Handle greyMaterial(scene::Scene & scene, const std::string & name, float grey, const std::string & shader = "matte") {
    const scene::Handle node = scene.create("shader", name);
    scene.set(node, "shader", scene::Value::string(shader));
    scene.set(node, "language", scene::Value::string("ri"));
    scene.declare(node, scene::Declaration { .name = "Cs", .type = scene::Type::Colour, .fallback = scene::Value::colour(glm::vec3 { 1.0F }) });
    scene.set(node, "Cs", scene::Value::colour(glm::vec3 { grey }));
    const scene::Handle material = scene.create("material", name);
    scene.set(material, "surface", scene::Value::handle(node));
    return material;
}

/**
 * A gathered material made one of closures alone, of a grey, in place of OpenPBR's layers, for a
 * case to set its closures. A geometry's Cs overrides the grey.
 **/
kernel::surface::Material & closured(geometry::Prototypes & prototypes, float grey) {
    kernel::surface::Material & out = prototypes.materials.front();
    out = kernel::surface::Material {};
    out.colour = glm::vec3 { grey };
    out.coloured = 1;
    return out;
}

/**
 * The scene the mirror case renders, under a light sampling strategy. It holds the camera of
 * framed() and a white sphere light of radius three and a radiance of one. The light lies fifteen
 * units ahead of the camera along z, or as far behind it with a square mirror at the origin facing
 * the camera. The mirror's material is a grey of 0.8, which code makes a mirror.
 **/
void buildMirrored(scene::Scene & scene, const std::string & strategy, bool mirrored) {
    const scene::Handle options = framed(scene, 16);
    scene.set(options, "lightSampling", scene::Value::string(strategy));
    const scene::Handle bulb = scene.create("sphereLight", "bulb");
    scene.set(bulb, "radius", scene::Value::number(3.0F));
    scene.set(bulb, "normalize", scene::Value::integer(0));
    scene.set(bulb, "camera", scene::Value::integer(1));
    scene.set(bulb, "transform", scene::Value::matrix(moved(0.0F, 0.0F, mirrored ? -15.0F : 15.0F)));
    if (mirrored) {
        const scene::Handle square = scene.create("mesh", "square");
        scene.set(square, "P", scene::Value::points({ { -3.0F, -3.0F, 0.0F }, { 3.0F, -3.0F, 0.0F }, { 3.0F, 3.0F, 0.0F }, { -3.0F, 3.0F, 0.0F } }));
        scene.set(square, "vertices", scene::Value::integers({ 4 }));
        scene.set(square, "indices", scene::Value::integers({ 0, 1, 2, 3 }));
        const scene::Handle placed = instanced(scene, "mirror", square, {}, glm::mat4(1.0F));
        scene.set(placed, "material", scene::Value::handle(greyMaterial(scene, "mirror", 0.8F)));
    }
    scene.commit();
    scene.apply();
}

/** Whether two snapshots hold the same floats, bit for bit, and the same counts of samples. **/
bool sameBytes(const film::Snapshot & one, const film::Snapshot & other) {
    return one.values.size() == other.values.size() && one.samples == other.samples &&
        std::memcmp(one.values.data(), other.values.data(), one.values.size() * sizeof(float)) == 0;
}

// a render takes its samples rounded up to a power of two, sixteen to a pass, and a scene whose
// options name no samples takes one
BOOST_AUTO_TEST_CASE(moya_paths_the_passes_a_render_takes_test) {
    const scene::Scene bare(scene::kinds());
    BOOST_TEST(v3d::moya::integrate::passesOf(bare) == 1U);
    for (const auto & [samples, passes] : { std::pair { 1U, 1U }, std::pair { 16U, 1U }, std::pair { 17U, 2U }, std::pair { 33U, 4U }, std::pair { 64U, 4U } }) {
        scene::Scene made(scene::kinds());
        framed(made, samples);
        made.commit();
        made.apply();
        BOOST_TEST(v3d::moya::integrate::passesOf(made) == passes, samples << " samples");
    }
}

// a white sphere under a white dome is white wherever it is seen: every path reaches the
// dome carrying all of its light
BOOST_AUTO_TEST_CASE(moya_paths_a_white_sphere_in_a_white_furnace_is_white_test) {
    const film::Snapshot snapshot = sphereIn(1.0F, 12);
    const film::Furthest furthest = film::furthest(snapshot, [](const glm::uvec2 &) { return 1.0F; });
    BOOST_TEST(furthest.sigmas < 5.0F);
    BOOST_TEST(furthest.unmeasured == 0U);
    BOOST_TEST(film::sigmas(film::mean(snapshot, { 0, 0 }, { kSide, kSide }), 1.0F) < 3.0F);
    BOOST_TEST(film::sigmas(film::mean(snapshot, glm::uvec2(kInsideFrom), glm::uvec2(kInsideTo)), 1.0F) < 3.0F);
}

// a convex grey sphere never meets itself, so it shows its albedo times the dome
BOOST_AUTO_TEST_CASE(moya_paths_a_grey_sphere_shows_its_albedo_test) {
    const film::Snapshot snapshot = sphereIn(0.5F, 12);
    BOOST_TEST(film::sigmas(film::mean(snapshot, glm::uvec2(kInsideFrom), glm::uvec2(kInsideTo)), 0.5F) < 3.0F);
    BOOST_TEST(film::sigmas(film::mean(snapshot, glm::uvec2(kOutsideFrom), glm::uvec2(kOutsideTo)), 1.0F) < 3.0F);
    BOOST_TEST(film::sigmas(film::mean(snapshot, glm::uvec2(kInsideFrom), glm::uvec2(kInsideTo)), 0.6F) > 10.0F);
}

// with no bounce allowed, a camera ray that meets the sphere gathers nothing, and one that misses
// it still meets the dome. A diffuse limit of zero ends the same paths
BOOST_AUTO_TEST_CASE(moya_paths_a_depth_limit_of_zero_shows_the_environment_alone_test) {
    const film::Snapshot snapshot = sphereIn(1.0F, 0);
    BOOST_TEST(film::mean(snapshot, glm::uvec2(kInsideFrom), glm::uvec2(kInsideTo)).value == 0.0F);
    BOOST_TEST(film::sigmas(film::mean(snapshot, glm::uvec2(kOutsideFrom), glm::uvec2(kOutsideTo)), 1.0F) < 3.0F);

    scene::Scene scene(scene::kinds());
    const scene::Handle options = framed(scene, 16);
    scene.set(options, "diffuseDepth", scene::Value::integer(0));
    ball(scene, "ball", 1.0F, glm::mat4(1.0F));
    scene.commit();
    scene.apply();
    CpuDevice device;
    BOOST_TEST(film::mean(integrate::render(device, scene, {}, colour::Config()), glm::uvec2(kInsideFrom), glm::uvec2(kInsideTo)).value == 0.0F);
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
    BOOST_TEST(film::mean(late, { 12, 26 }, { 20, 30 }).value < film::mean(late, glm::uvec2(kOutsideFrom), glm::uvec2(kOutsideTo)).value);
}

BOOST_AUTO_TEST_CASE(moya_paths_a_picture_is_the_same_on_one_thread_and_on_every_thread_test) {
    const film::Snapshot one = sphereOnPlane(3, 1);
    const film::Snapshot every = sphereOnPlane(3, 0);
    BOOST_TEST(one.values == every.values, boost::test_tools::per_element());
}

// the dome seen directly is nearly flat, its noise only that of the wavelengths drawn, so its
// pixels stop within two passes, while the floor in the sphere's shadow keeps sampling
BOOST_AUTO_TEST_CASE(moya_paths_adaptive_sampling_stops_the_flat_pixels_first_test) {
    const film::Snapshot snapshot = sphereOnPlane(3, 0, kThreshold);
    BOOST_TEST(snapshot.count(0, 0) <= 2U * integrate::kSamplesPerPass);
    BOOST_TEST(snapshot.count(16, 30) > 8U * integrate::kSamplesPerPass);
    // with no threshold every pixel takes every sample
    const film::Snapshot full = sphereOnPlane(3, 0);
    BOOST_TEST(full.count(0, 0) == 256U);
    BOOST_TEST(full.count(16, 30) == 256U);
}

// a picture sampled adaptively differs from one sampled to the most by less than the threshold,
// measured as the noise is, on average over its pixels
BOOST_AUTO_TEST_CASE(moya_paths_an_adaptive_picture_is_within_its_threshold_test) {
    const film::Snapshot adaptive = sphereOnPlane(3, 0, kThreshold);
    const film::Snapshot full = sphereOnPlane(3, 0);
    double sum = 0.0;
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            float apart = 0.0F;
            float total = 0.0F;
            for (std::uint32_t i = 0; i < 3; ++i) {
                apart += std::abs(adaptive.at(x, y, i) - full.at(x, y, i));
                total += full.at(x, y, i);
            }
            sum += apart / (1.0e-4F + std::sqrt(total));
        }
    }
    BOOST_TEST(sum / (kSide * kSide) < kThreshold);
}

// the same pixels stop, after the same samples, on one thread and on every thread
BOOST_AUTO_TEST_CASE(moya_paths_adaptive_sampling_is_the_same_on_one_thread_and_on_every_thread_test) {
    const film::Snapshot one = sphereOnPlane(3, 1, kThreshold);
    const film::Snapshot every = sphereOnPlane(3, 0, kThreshold);
    BOOST_TEST(one.samples == every.samples, boost::test_tools::per_element());
    BOOST_TEST(one.values == every.values, boost::test_tools::per_element());
}

// a render stopped after two passes and resumed to eight, on another thread count, is the same,
// byte for byte, as one of eight passes that never stopped
BOOST_AUTO_TEST_CASE(moya_paths_a_resumed_render_is_the_same_as_one_that_never_stopped_test) {
    const std::string path = checkpointNamed("resumed");
    const film::Snapshot whole = checkpointed(0.0F, 0, {});
    BOOST_TEST(whole.passes == 8U);
    const film::Snapshot stopped = checkpointed(0.0F, 0, { .path = path, .stop = [](std::uint32_t passes) { return passes == 2U; } });
    BOOST_TEST(stopped.passes == 2U);
    BOOST_TEST(output::readCheckpoint(path, {}, printOf(0.0F)).passes == 2U);
    const film::Snapshot resumed = checkpointed(0.0F, 1, { .path = path, .resume = true });
    BOOST_TEST(resumed.passes == 8U);
    BOOST_TEST(sameBytes(resumed, whole));
    BOOST_TEST(!sameBytes(stopped, whole));
    boost::system::error_code ignored;
    boost::filesystem::remove(path, ignored);
}

// a render that adaptive sampling stopped pixels of keeps them stopped in its checkpoint, and
// resumed it stops the same pixels after the same samples as one that never stopped
BOOST_AUTO_TEST_CASE(moya_paths_a_resumed_render_keeps_its_stopped_pixels_test) {
    const std::string path = checkpointNamed("adaptive");
    const film::Snapshot whole = checkpointed(kThreshold, 0, {});
    checkpointed(kThreshold, 0, { .path = path, .stop = [](std::uint32_t passes) { return passes == 4U; } });
    std::uint32_t stopped = 0;
    for (const std::uint32_t flag : output::readCheckpoint(path, {}, printOf(kThreshold)).stopped) {
        stopped += flag != 0U ? 1U : 0U;
    }
    BOOST_TEST(stopped > 0U);
    const film::Snapshot resumed = checkpointed(kThreshold, 0, { .path = path, .resume = true });
    BOOST_TEST(sameBytes(resumed, whole));
    boost::system::error_code ignored;
    boost::filesystem::remove(path, ignored);
}

// a checkpoint of a scene is refused by the same scene changed, which has another fingerprint
BOOST_AUTO_TEST_CASE(moya_paths_a_resume_against_a_changed_scene_is_refused_test) {
    const std::string path = checkpointNamed("changed");
    checkpointed(0.0F, 0, { .path = path, .stop = [](std::uint32_t passes) { return passes == 1U; } });
    BOOST_CHECK_THROW(checkpointed(0.0F, 0, { .path = path, .resume = true }, 0.5F), output::Refused);
    BOOST_CHECK_NO_THROW(checkpointed(0.0F, 0, { .path = path, .resume = true }));

    scene::Scene first(scene::kinds());
    scene::Scene again(scene::kinds());
    scene::Scene changed(scene::kinds());
    buildSphereOnPlane(first, 128, 3, 0.0F);
    buildSphereOnPlane(again, 128, 3, 0.0F);
    buildSphereOnPlane(changed, 128, 3, 0.0F, 0.5F);
    BOOST_TEST(integrate::fingerprint(first, {}) == integrate::fingerprint(again, {}));
    BOOST_TEST(integrate::fingerprint(first, {}) != integrate::fingerprint(changed, {}));
    BOOST_TEST(integrate::fingerprint(first, {}) != integrate::fingerprint(first, { { .name = "depth", .components = 1 } }));
    boost::system::error_code ignored;
    boost::filesystem::remove(path, ignored);
}

// a counted render gives the same picture as one that is not. Under the dome alone every camera
// ray leaves the scene, so each sample traces one ray for its nearest hit and no shadow ray
BOOST_AUTO_TEST_CASE(moya_paths_a_counted_render_counts_its_rays_test) {
    scene::Scene empty(scene::kinds());
    framed(empty, 32);
    empty.commit();
    empty.apply();
    CpuDevice device;
    integrate::Profile counted { .counted = true };
    integrate::render(device, empty, {}, colour::Config(), {}, &counted);
    BOOST_TEST(counted.nearest == std::uint64_t { kSide } * kSide * 32U);
    BOOST_TEST(counted.any == 0U);
    BOOST_TEST(counted.seconds > 0.0);

    scene::Scene lit(scene::kinds());
    buildSphereOnPlane(lit, 32, 3, 0.0F);
    integrate::Profile timed;
    const film::Snapshot plain = integrate::render(device, lit, {}, colour::Config(), {}, &timed);
    integrate::Profile profiled { .counted = true };
    const film::Snapshot measured = integrate::render(device, lit, {}, colour::Config(), {}, &profiled);
    BOOST_TEST(sameBytes(plain, measured));
    BOOST_TEST(timed.nearest == 0U);
    BOOST_TEST(profiled.nearest > std::uint64_t { kSide } * kSide * 32U);
    BOOST_TEST(profiled.any > 0U);
    BOOST_TEST(profiled.traversal > 0.0);
    BOOST_TEST(profiled.traversal < 1.0);
}

// the settings are the options', each depth held to zero or more
BOOST_AUTO_TEST_CASE(moya_paths_the_settings_are_the_options_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle options = scene.create("options", "options");
    scene.set(options, "maxDepth", scene::Value::integer(7));
    scene.set(options, "diffuseDepth", scene::Value::integer(-2));
    scene.set(options, "glossyDepth", scene::Value::integer(3));
    scene.set(options, "specularDepth", scene::Value::integer(5));
    scene.set(options, "transmissionDepth", scene::Value::integer(6));
    scene.set(options, "rouletteDepth", scene::Value::integer(2));
    scene.commit();
    scene.apply();
    const v3d::moya::kernel::integrate::Settings settings = integrate::settingsOf(scene);
    BOOST_TEST(settings.maxDepth == 7U);
    BOOST_TEST(settings.diffuseDepth == 0U);
    BOOST_TEST(settings.glossyDepth == 3U);
    BOOST_TEST(settings.specularDepth == 5U);
    BOOST_TEST(settings.transmissionDepth == 6U);
    BOOST_TEST(settings.rouletteDepth == 2U);

    const scene::Scene empty(scene::kinds());
    const v3d::moya::kernel::integrate::Settings fallback = integrate::settingsOf(empty);
    BOOST_TEST(fallback.maxDepth == 12U);
    BOOST_TEST(fallback.diffuseDepth == 4U);
    BOOST_TEST(fallback.rouletteDepth == 3U);
}

// a surface of two diffuse closures of half its colour each renders as one of the whole, within
// its errors. Rendering the prototypes gathered from a scene gives the scene's own picture
BOOST_AUTO_TEST_CASE(moya_paths_two_halves_of_a_diffuse_lobe_render_as_one_test) {
    scene::Scene scene(scene::kinds());
    buildSphereOnPlane(scene, 256, 3, 0.0F);
    const scene::Handle material = greyMaterial(scene, "grey", 0.5F);
    for (const scene::Handle & instance : scene.objects("instance")) {
        scene.set(instance, "material", scene::Value::handle(material));
    }
    scene.commit();
    scene.apply();
    CpuDevice device;
    geometry::Prototypes prototypes = geometry::gather(scene, {});
    const film::Snapshot whole = integrate::render(device, scene, prototypes, {}, colour::Config());
    BOOST_TEST(sameBytes(whole, integrate::render(device, scene, {}, colour::Config())));
    BOOST_TEST_REQUIRE(prototypes.materials.size() == 1U);
    kernel::surface::Material & halved = closured(prototypes, 0.5F);
    halved.closures[0] = kernel::surface::MaterialClosure { .model = kernel::bsdf::Model::Diffuse, .scale = 0.5F };
    halved.closures[1] = kernel::surface::MaterialClosure { .model = kernel::bsdf::Model::Diffuse, .scale = 0.5F };
    halved.count = 2;
    const film::Snapshot halves = integrate::render(device, scene, prototypes, {}, colour::Config());
    BOOST_TEST(!sameBytes(whole, halves));
    const film::Furthest furthest = film::furthest(whole, halves);
    BOOST_TEST(furthest.sigmas < 5.0F);
    BOOST_TEST(furthest.unmeasured == 0U);
}

// a light seen in a perfect mirror shows the mirror's reflectance times the light, under every
// strategy. No light sample can draw the mirror's direction, so the light it meets takes the
// sample's whole weight
BOOST_AUTO_TEST_CASE(moya_paths_a_mirror_shows_a_light_with_its_whole_weight_test) {
    CpuDevice device;
    for (const char * strategy : { "mis", "light", "bsdf" }) {
        BOOST_TEST_CONTEXT(strategy) {
            scene::Scene direct(scene::kinds());
            buildMirrored(direct, strategy, false);
            const film::Estimate seen = film::mean(integrate::render(device, direct, {}, colour::Config()), glm::uvec2(kInsideFrom), glm::uvec2(kInsideTo));
            BOOST_TEST_REQUIRE(seen.value > 0.5F);

            scene::Scene mirrored(scene::kinds());
            buildMirrored(mirrored, strategy, true);
            geometry::Prototypes prototypes = geometry::gather(mirrored, {});
            BOOST_TEST_REQUIRE(prototypes.materials.size() == 1U);
            closured(prototypes, 0.8F).closures[0].model = kernel::bsdf::Model::Mirror;
            const film::Estimate reflected =
                film::mean(integrate::render(device, mirrored, prototypes, {}, colour::Config()), glm::uvec2(kInsideFrom), glm::uvec2(kInsideTo));
            BOOST_TEST(reflected.value == 0.8F * seen.value, boost::test_tools::tolerance(0.01F));
        }
    }
}

// a white rough metal sphere under a white dome is white wherever it is seen: its compensation
// gives back what a single scattering loses, at every angle the camera sees it from
BOOST_AUTO_TEST_CASE(moya_paths_a_white_metal_sphere_in_a_white_furnace_is_white_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle options = framed(scene, 64);
    scene.set(options, "glossyDepth", scene::Value::integer(12));
    const scene::Handle placed = ball(scene, "ball", 1.0F, glm::mat4(1.0F));
    scene.set(placed, "material", scene::Value::handle(greyMaterial(scene, "metal", 1.0F)));
    scene.commit();
    scene.apply();
    geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST_REQUIRE(prototypes.materials.size() == 1U);
    closured(prototypes, 1.0F).closures[0] =
        kernel::surface::MaterialClosure { .model = kernel::bsdf::Model::Conductor, .scale = 1.0F, .roughness = 0.7F, .anisotropy = 0.0F, .tint = glm::vec3 { 1.0F } };
    CpuDevice device;
    const film::Snapshot snapshot = integrate::render(device, scene, prototypes, {}, colour::Config());
    const film::Furthest furthest = film::furthest(snapshot, [](const glm::uvec2 &) { return 1.0F; });
    BOOST_TEST(furthest.sigmas < 5.0F);
    BOOST_TEST(film::mean(snapshot, glm::uvec2(kInsideFrom), glm::uvec2(kInsideTo)).value == 1.0F, boost::test_tools::tolerance(0.01F));
}

// a clear sphere under a white dome vanishes: light that enters it, is trapped inside it and
// leaves it carries all of the dome's light back, smooth or rough, and as a thin-walled bubble
BOOST_AUTO_TEST_CASE(moya_paths_a_clear_sphere_in_a_white_furnace_vanishes_test) {
    for (const float roughness : { 0.0F, 0.5F }) {
        for (const bool thin : { false, true }) {
            BOOST_TEST_CONTEXT("roughness " << roughness << ", thin " << thin) {
                scene::Scene scene(scene::kinds());
                const scene::Handle options = framed(scene, 64);
                scene.set(options, "maxDepth", scene::Value::integer(64));
                scene.set(options, "specularDepth", scene::Value::integer(64));
                scene.set(options, "glossyDepth", scene::Value::integer(64));
                scene.set(options, "transmissionDepth", scene::Value::integer(64));
                const scene::Handle placed = ball(scene, "ball", 1.0F, glm::mat4(1.0F));
                scene.set(placed, "material", scene::Value::handle(greyMaterial(scene, "glass", 1.0F)));
                scene.commit();
                scene.apply();
                geometry::Prototypes prototypes = geometry::gather(scene, {});
                BOOST_TEST_REQUIRE(prototypes.materials.size() == 1U);
                closured(prototypes, 1.0F).closures[0] =
                    kernel::surface::MaterialClosure { .model = kernel::bsdf::Model::Dielectric, .scale = 1.0F, .roughness = roughness, .ior = 1.5F, .thin = thin };
                CpuDevice device;
                const film::Snapshot snapshot = integrate::render(device, scene, prototypes, {}, colour::Config());
                const film::Furthest furthest = film::furthest(snapshot, [](const glm::uvec2 &) { return 1.0F; });
                BOOST_TEST(furthest.sigmas < 5.0F);
                BOOST_TEST(film::mean(snapshot, glm::uvec2(kInsideFrom), glm::uvec2(kInsideTo)).value == 1.0F, boost::test_tools::tolerance(0.01F));
            }
        }
    }
}

// RI's matte, metal and glass, white, are OpenPBR's layers, and are white in a white furnace:
// each conserves the dome's light
BOOST_AUTO_TEST_CASE(moya_paths_ris_white_surfaces_in_a_white_furnace_are_white_test) {
    for (const char * shader : { "matte", "metal", "glass" }) {
        BOOST_TEST_CONTEXT(shader) {
            scene::Scene scene(scene::kinds());
            const scene::Handle options = framed(scene, 64);
            for (const char * depth : { "maxDepth", "diffuseDepth", "specularDepth", "glossyDepth", "transmissionDepth" }) {
                scene.set(options, depth, scene::Value::integer(64));
            }
            const scene::Handle placed = ball(scene, "ball", 1.0F, glm::mat4(1.0F));
            scene.set(placed, "material", scene::Value::handle(greyMaterial(scene, shader, 1.0F, shader)));
            scene.commit();
            scene.apply();
            const geometry::Prototypes gathered = geometry::gather(scene, {});
            BOOST_TEST_REQUIRE(gathered.materials.size() == 1U);
            BOOST_TEST(gathered.materials.front().layered == 1U);
            CpuDevice device;
            const film::Snapshot snapshot = integrate::render(device, scene, {}, colour::Config());
            const film::Furthest furthest = film::furthest(snapshot, [](const glm::uvec2 &) { return 1.0F; });
            BOOST_TEST(furthest.sigmas < 5.0F);
            BOOST_TEST(film::mean(snapshot, glm::uvec2(kInsideFrom), glm::uvec2(kInsideTo)).value == 1.0F, boost::test_tools::tolerance(0.01F));
        }
    }
}

// a white OpenPBR sphere under a white dome is white wherever it is seen: its coat, its metal, its
// specular layer and its rough diffuse base give back all of the dome's light between them
BOOST_AUTO_TEST_CASE(moya_paths_a_white_openpbr_sphere_in_a_white_furnace_is_white_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle options = framed(scene, 64);
    scene.set(options, "maxDepth", scene::Value::integer(32));
    scene.set(options, "diffuseDepth", scene::Value::integer(32));
    scene.set(options, "glossyDepth", scene::Value::integer(32));
    scene.set(options, "specularDepth", scene::Value::integer(32));
    const scene::Handle placed = ball(scene, "ball", 1.0F, glm::mat4(1.0F));
    scene.set(placed, "material", scene::Value::handle(greyMaterial(scene, "layered", 1.0F)));
    scene.commit();
    scene.apply();
    geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST_REQUIRE(prototypes.materials.size() == 1U);
    kernel::surface::Material & material = prototypes.materials.front();
    material.layered = 1U;
    material.surface.baseColour = glm::vec3 { 1.0F };
    material.surface.baseDiffuseRoughness = 0.5F;
    material.surface.baseMetalness = 0.3F;
    material.surface.specularRoughness = 0.4F;
    material.surface.coatWeight = 1.0F;
    material.surface.coatRoughness = 0.2F;
    CpuDevice device;
    const film::Snapshot snapshot = integrate::render(device, scene, prototypes, {}, colour::Config());
    const film::Furthest furthest = film::furthest(snapshot, [](const glm::uvec2 &) { return 1.0F; });
    BOOST_TEST(furthest.sigmas < 5.0F);
    BOOST_TEST(film::mean(snapshot, glm::uvec2(kInsideFrom), glm::uvec2(kInsideTo)).value == 1.0F, boost::test_tools::tolerance(0.02F));
}

};  // namespace

};  // namespace v3d::moya::tracing

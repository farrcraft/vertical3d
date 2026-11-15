/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/log/Logger.h>
#include <moya/libmoya/accel/Assembler.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Compare.h>
#include <moya/libmoya/film/Estimate.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/geometry/Group.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/Surfaces.h>
#include <moya/libmoya/geometry/World.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/Dome.h>
#include <moya/libmoya/kernel/light/Emission.h>
#include <moya/libmoya/kernel/light/Lights.h>
#include <moya/libmoya/kernel/light/Picked.h>
#include <moya/libmoya/kernel/light/Tree.h>
#include <moya/libmoya/kernel/program/Program.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/light/Ies.h>
#include <moya/libmoya/light/Lights.h>
#include <moya/libmoya/light/Planckian.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Placement.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <OpenEXR/ImfEnvmap.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfRgba.h>
#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImfStandardAttributes.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <ios>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Handling.h"
#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace colour = v3d::moya::colour;
namespace film = v3d::moya::film;
namespace geometry = v3d::moya::geometry;
namespace integrate = v3d::moya::integrate;
namespace kernel = v3d::moya::kernel;
namespace light = v3d::moya::light;
namespace scene = v3d::moya::scene;
typedef v3d::moya::device::CpuDevice CpuDevice;

constexpr float kPi = 3.14159265358979323846F;

/** The side of each picture, in pixels. **/
constexpr std::uint32_t kSide = 32;

/** The first pixel of the middle of a picture along each axis, and the one past its last. **/
constexpr std::uint32_t kMiddleFrom = 14;
constexpr std::uint32_t kMiddleTo = 18;

/** The texel a bright texel test lights, of an image of kDomeWidth by kDomeHeight. **/
constexpr std::uint32_t kDomeWidth = 64;
constexpr std::uint32_t kDomeHeight = 32;
constexpr std::uint32_t kLitColumn = 6;
constexpr std::uint32_t kLitRow = 12;

/** A scene's lights, built on a device of some threads, with the surfaces that place them. **/
struct Built {
    colour::Config config;
    CpuDevice device;
    geometry::Prototypes prototypes;
    geometry::Surfaces surfaces;
    light::Lights lights;

    Built(const scene::Scene & scene, unsigned int threads)
        : device(threads),
          prototypes(geometry::gather(scene, {})),
          surfaces(device, prototypes, config),
          lights(device, scene, prototypes, config, 10.0F, {}) {}
};

/** A half turn about x, which turns -z onto +z. **/
glm::mat4 flipped(const glm::mat4 & place) {
    return glm::rotate(place, glm::radians(180.0F), glm::vec3 { 1.0F, 0.0F, 0.0F });
}

/**
 * A scene of an orthographic camera at a distance back along -z, which looks along +z. It sees
 * the square of a half width about a centre in x and y, at kSide pixels square, with samples per
 * pixel and no dome. Gives the options.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the samples, then the view
scene::Handle viewed(scene::Scene & scene, std::uint32_t samples, float distance, const glm::vec2 & centre, float half) {
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(options, "resolution", scene::Value::integers({ kSide, kSide }));
    scene.set(options, "samples", scene::Value::integer(static_cast<std::int32_t>(samples)));
    scene.set(camera, "screen", scene::Value::floats({ centre.x - half, centre.x + half, centre.y - half, centre.y + half }));
    scene.set(camera, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -distance)));
    return options;
}

/** A white plane at z = 0 that faces the camera. **/
void plane(scene::Scene & scene) {
    const scene::Handle floor = scene.create("disk", "floor");
    scene.set(floor, "radius", scene::Value::number(100.0F));
    scene.declare(floor, scene::Declaration { .name = "Cs",
        .type = scene::Type::Colour,
        .count = scene::Declaration::kAny,
        .fallback = scene::Value::colours({}),
        .interpolation = scene::Interpolation::Constant });
    scene.set(floor, "Cs", scene::Value::colours({ glm::vec3 { 1.0F } }));
    // the disk faces +z in its own space, so it is turned to face the camera
    instanced(scene, "floor", floor, {}, flipped(glm::mat4(1.0F)));
}

/** A disk light of a radius at a height above the plane, facing it. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the radius, then the height
scene::Handle diskAbove(scene::Scene & scene, float radius, float height) {
    const scene::Handle disk = scene.create("diskLight", "key");
    scene.set(disk, "radius", scene::Value::number(radius));
    scene.set(disk, "transform", scene::Value::matrix(flipped(moved(0.0F, 0.0F, -height))));
    return disk;
}

film::Snapshot rendered(scene::Scene & scene) {
    scene.commit();
    scene.apply();
    CpuDevice device;
    return integrate::render(device, scene, {}, colour::Config());
}

/**
 * The radiance a white diffuse plane sends back from a point a distance off a disk light's axis:
 * its irradiance over pi. The light has a radiance of one, and faces the plane from a height.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the distance, the radius, then the height
float underDisk(float off, float radius, float height) {
    const float h2 = height * height;
    const float d2 = off * off;
    const float r2 = radius * radius;
    return 0.5F * (1.0F - ((h2 + d2 - r2) / std::sqrt(((h2 + d2 + r2) * (h2 + d2 + r2)) - (4.0F * r2 * d2))));
}

/** The mean over every pixel of what is expected at its centre, as a camera views a square. **/
float meanOver(const glm::vec2 & centre, float half, float (*expected)(const glm::vec2 &, float, float), float radius, float height) {
    double sum = 0.0;
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            const glm::vec2 at = centre - half + (glm::vec2 { static_cast<float>(x), static_cast<float>(y) } + 0.5F) * (2.0F * half / static_cast<float>(kSide));
            sum += expected(at, radius, height);
        }
    }
    return static_cast<float>(sum / (kSide * kSide));
}

float diskAt(const glm::vec2 & at, float radius, float height) {
    return underDisk(glm::length(at), radius, height);
}

/**
 * A white plane under a disk light of a radius at a height, lit by a strategy and viewed from
 * five units back across half a unit, with samples per pixel.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the radius, then the height
film::Snapshot diskPicture(const char * strategy, float radius, float height, std::uint32_t samples) {
    scene::Scene scene(scene::kinds());
    const scene::Handle options = viewed(scene, samples, 5.0F, glm::vec2 { 0.0F }, 0.25F);
    scene.set(options, "lightSampling", scene::Value::string(strategy));
    plane(scene);
    diskAbove(scene, radius, height);
    return rendered(scene);
}

/** A light camera rays see, at the origin, in a scene viewed from five units back. **/
scene::Handle seen(scene::Scene & scene, const std::string & kind, const glm::mat4 & place) {
    viewed(scene, 16, 5.0F, glm::vec2 { 0.0F }, 1.0F);
    const scene::Handle made = scene.create(kind, "seen");
    scene.set(made, "camera", scene::Value::integer(1));
    scene.set(made, "normalize", scene::Value::integer(1));
    scene.set(made, "transform", scene::Value::matrix(place));
    return made;
}

// a white plane under a disk light reaches the closed form of the disk's irradiance by each
// strategy. Paths find the light by the plane's BSDF, by light samples alone, and by both. The
// camera sees through the light
BOOST_AUTO_TEST_CASE(moya_lights_a_disk_light_gives_its_closed_form_by_each_strategy_test) {
    const glm::vec2 centre { 0.0F };
    const float expected = meanOver(centre, 0.25F, &diskAt, 2.0F, 1.0F);
    for (const char * strategy : { "bsdf", "light", "mis" }) {
        BOOST_TEST_CONTEXT(strategy) {
            const film::Snapshot snapshot = diskPicture(strategy, 2.0F, 1.0F, 256);
            const film::Furthest furthest = film::furthest(snapshot, [&](const glm::uvec2 & pixel) {
                const glm::vec2 at =
                    centre - 0.25F + (glm::vec2 { static_cast<float>(pixel.x), static_cast<float>(pixel.y) } + 0.5F) * (0.5F / static_cast<float>(kSide));
                return underDisk(glm::length(at), 2.0F, 1.0F);
            });
            BOOST_TEST(furthest.sigmas < 5.0F);
            BOOST_TEST(film::sigmas(film::mean(snapshot, { 0, 0 }, { kSide, kSide }), expected) < 3.0F);
            BOOST_TEST(film::sigmas(film::mean(snapshot, { 0, 0 }, { kSide, kSide }), expected * 1.05F) > 10.0F);
        }
    }
}

// multiple importance sampling keeps a standard error near the better strategy's, and far below
// the worse one's. Light samples find a small light best, and the BSDF a large one. It is not
// below the better one in every scene, since the power heuristic splits each direction between
// two samples
BOOST_AUTO_TEST_CASE(moya_lights_mis_keeps_near_the_better_strategy_for_a_small_light_and_a_large_one_test) {
    const auto error = [](const char * strategy, float radius) {
        return film::mean(diskPicture(strategy, radius, 1.0F, 64), { 0, 0 }, { kSide, kSide }).error;
    };
    for (const float radius : { 0.2F, 8.0F }) {
        BOOST_TEST_CONTEXT("radius " << radius) {
            const float bsdf = error("bsdf", radius);
            const float lit = error("light", radius);
            const float both = error("mis", radius);
            BOOST_TEST(both <= 2.0F * std::fmin(bsdf, lit));
            BOOST_TEST(both <= 0.6F * std::fmax(bsdf, lit));
        }
    }
}

// a light sample is shadowed by what lies between: a plane under a disk light, with a disk wider
// than the plane between them, is black by light samples alone. The camera sees through the disk
BOOST_AUTO_TEST_CASE(moya_lights_a_light_sample_is_shadowed_by_what_lies_between_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle options = viewed(scene, 16, 5.0F, glm::vec2 { 0.0F }, 0.25F);
    plane(scene);
    diskAbove(scene, 0.5F, 2.0F);
    scene.set(options, "lightSampling", scene::Value::string("light"));
    const scene::Handle cover = scene.create("disk", "cover");
    scene.set(cover, "radius", scene::Value::number(200.0F));
    scene.set(instanced(scene, "cover", cover, {}, moved(0.0F, 0.0F, -1.0F)), "camera", scene::Value::integer(0));
    BOOST_TEST(film::mean(rendered(scene), { 0, 0 }, { kSide, kSide }).value == 0.0F);
}

// a bounce a depth limit refuses samples no light. By light samples alone, the plane under a disk
// light is lit, and black with no bounce allowed, or no diffuse one
BOOST_AUTO_TEST_CASE(moya_lights_a_depth_limit_stops_light_samples_test) {
    const auto lit = [](const char * limit, std::int32_t depth) {
        scene::Scene scene(scene::kinds());
        const scene::Handle options = viewed(scene, 16, 5.0F, glm::vec2 { 0.0F }, 0.25F);
        scene.set(options, "lightSampling", scene::Value::string("light"));
        scene.set(options, limit, scene::Value::integer(depth));
        plane(scene);
        diskAbove(scene, 2.0F, 1.0F);
        return film::mean(rendered(scene), { 0, 0 }, { kSide, kSide }).value;
    };
    BOOST_TEST(lit("maxDepth", 12) > 0.5F);
    BOOST_TEST(lit("maxDepth", 0) == 0.0F);
    BOOST_TEST(lit("diffuseDepth", 0) == 0.0F);
}

// light samples alone and BSDF samples alone give the same picture of a plane lit by a rect, a
// sphere and a cylinder light, each sampled its own way
BOOST_AUTO_TEST_CASE(moya_lights_each_shape_gives_one_picture_by_light_samples_and_by_the_bsdf_test) {
    const auto lit = [](const char * strategy) {
        scene::Scene scene(scene::kinds());
        const scene::Handle options = viewed(scene, 256, 5.0F, glm::vec2 { 0.0F }, 1.5F);
        plane(scene);
        scene.set(options, "lightSampling", scene::Value::string(strategy));
        const scene::Handle rect = scene.create("rectLight", "rect");
        scene.set(rect, "transform", scene::Value::matrix(glm::rotate(moved(-0.8F, 0.6F, -1.0F), glm::radians(160.0F), glm::vec3 { 1.0F, 0.2F, 0.0F })));
        const scene::Handle sphere = scene.create("sphereLight", "sphere");
        scene.set(sphere, "radius", scene::Value::number(0.3F));
        scene.set(sphere, "transform", scene::Value::matrix(moved(0.8F, 0.6F, -0.8F)));
        const scene::Handle cylinder = scene.create("cylinderLight", "cylinder");
        scene.set(cylinder, "radius", scene::Value::number(0.15F));
        scene.set(cylinder, "transform", scene::Value::matrix(moved(0.0F, -0.8F, -0.6F)));
        return rendered(scene);
    };
    const film::Snapshot bsdf = lit("bsdf");
    const film::Snapshot light = lit("light");
    BOOST_TEST(film::sigmas(film::mean(bsdf, { 0, 0 }, { kSide, kSide }), film::mean(light, { 0, 0 }, { kSide, kSide })) < 3.0F);
    BOOST_TEST(film::furthest(bsdf, light).sigmas < 5.0F);
    BOOST_TEST(film::mean(light, { 0, 0 }, { kSide, kSide }).value > 0.0F);
}

// with normalize, a disk light's radiance is divided by its area, so one of twice the radius
// gives the same irradiance far away. Each matches its own closed form
BOOST_AUTO_TEST_CASE(moya_lights_a_normalised_light_has_the_same_power_at_any_size_test) {
    const auto lit = [](float radius) {
        scene::Scene scene(scene::kinds());
        viewed(scene, 512, 10.0F, glm::vec2 { 0.0F }, 0.25F);
        plane(scene);
        const scene::Handle disk = diskAbove(scene, radius, 8.0F);
        scene.set(disk, "normalize", scene::Value::integer(1));
        return film::mean(rendered(scene), { 0, 0 }, { kSide, kSide });
    };
    const film::Estimate small = lit(0.5F);
    const film::Estimate large = lit(1.0F);
    BOOST_TEST(film::sigmas(small, meanOver(glm::vec2 { 0.0F }, 0.25F, &diskAt, 0.5F, 8.0F) / (kPi * 0.25F)) < 3.0F);
    BOOST_TEST(film::sigmas(large, meanOver(glm::vec2 { 0.0F }, 0.25F, &diskAt, 1.0F, 8.0F) / kPi) < 3.0F);
    // at eight units, the closed forms of the two lie 1.2% apart
    BOOST_TEST(std::fabs((small.value / large.value) - 1.0F) < 0.02F);
}

// a light lights nothing outside its cone: a point on the plane far off the light's axis sees it
// at more than sixty degrees, past a cone of ten
BOOST_AUTO_TEST_CASE(moya_lights_a_light_outside_its_cone_lights_nothing_test) {
    const auto lit = [](float cone) {
        scene::Scene scene(scene::kinds());
        viewed(scene, 64, 5.0F, glm::vec2 { 2.5F, 0.0F }, 0.25F);
        plane(scene);
        const scene::Handle disk = diskAbove(scene, 0.25F, 1.0F);
        scene.set(disk, "coneAngle", scene::Value::number(cone));
        return film::mean(rendered(scene), { 0, 0 }, { kSide, kSide });
    };
    BOOST_TEST(lit(10.0F).value == 0.0F);
    BOOST_TEST(lit(90.0F).value > 0.0F);
}

// the cone lets out all of the light inside the angle times one less the softness, falls
// smoothly to nothing at the angle, and shapes nothing at its fallback of ninety degrees
BOOST_AUTO_TEST_CASE(moya_lights_a_cone_falls_smoothly_to_its_edge_test) {
    kernel::light::Light shaped;
    shaped.coneAngle = glm::radians(60.0F);
    shaped.coneSoftness = 0.5F;
    BOOST_TEST(kernel::light::coneShare(shaped, std::cos(glm::radians(20.0F))) == 1.0F);
    BOOST_TEST(std::fabs(kernel::light::coneShare(shaped, std::cos(glm::radians(45.0F))) - 0.5F) < 1.0e-4F);
    BOOST_TEST(kernel::light::coneShare(shaped, std::cos(glm::radians(61.0F))) == 0.0F);
    const kernel::light::Light open;
    BOOST_TEST(kernel::light::coneShare(open, std::cos(glm::radians(120.0F))) == 1.0F);
}

// a Planckian radiator at 6504 K lies near D65, at a luminance of one
BOOST_AUTO_TEST_CASE(moya_lights_a_colour_temperature_of_6504_kelvin_is_near_d65_test) {
    const light::Planckian radiator = light::planckian(6504.0F);
    const glm::vec3 xyz = kernel::spectrum::acescgToXyz(radiator.colour);
    BOOST_TEST(std::fabs(xyz.y - 1.0F) < 1.0e-3F);
    const float sum = xyz.x + xyz.y + xyz.z;
    BOOST_TEST(glm::distance(glm::vec2 { xyz.x / sum, xyz.y / sum }, glm::vec2 { 0.3127F, 0.3290F }) < 0.01F);
    // the warm end is redder than the cool end
    const glm::vec3 warm = light::planckian(2000.0F).colour;
    const glm::vec3 cool = light::planckian(10000.0F).colour;
    BOOST_TEST(warm.x / warm.z > cool.x / cool.z);
    BOOST_TEST(light::planckian(0.0F).scale == 0.0F);
}

// a light with a colour temperature shines at its intensity, since its radiator's luminance is one
BOOST_AUTO_TEST_CASE(moya_lights_a_light_with_a_colour_temperature_keeps_its_luminance_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle sphere = seen(scene, "sphereLight", glm::mat4(1.0F));
    scene.set(sphere, "normalize", scene::Value::integer(0));
    scene.set(sphere, "enableColorTemperature", scene::Value::integer(1));
    scene.set(sphere, "colorTemperature", scene::Value::number(1000.0F));
    BOOST_TEST(film::sigmas(film::mean(rendered(scene), { kMiddleFrom, kMiddleFrom }, { kMiddleTo, kMiddleTo }), 1.0F) < 3.0F);
}

// camera rays see a light's shape only when the scene sets its camera visibility, and a rect
// light shows nothing from behind
BOOST_AUTO_TEST_CASE(moya_lights_camera_rays_see_a_light_only_when_set_test) {
    const auto rect = [](bool camera, const glm::mat4 & place) {
        scene::Scene scene(scene::kinds());
        const scene::Handle made = seen(scene, "rectLight", place);
        scene.set(made, "camera", scene::Value::integer(camera ? 1 : 0));
        scene.set(made, "normalize", scene::Value::integer(0));
        return film::mean(rendered(scene), { kMiddleFrom, kMiddleFrom }, { kMiddleTo, kMiddleTo });
    };
    BOOST_TEST(film::sigmas(rect(true, glm::mat4(1.0F)), 1.0F) < 3.0F);
    BOOST_TEST(rect(false, glm::mat4(1.0F)).value == 0.0F);
    BOOST_TEST(rect(true, flipped(glm::mat4(1.0F))).value == 0.0F);
}

// normalize divides by the area in the world: a rect's under a scale, a sphere's stretched into a
// spheroid, and a cylinder's. The cylinder lies along x
BOOST_AUTO_TEST_CASE(moya_lights_each_shape_is_normalised_by_its_area_in_the_world_test) {
    {
        scene::Scene scene(scene::kinds());
        seen(scene, "rectLight", glm::scale(glm::mat4(1.0F), glm::vec3 { 2.0F, 3.0F, 1.0F }));
        BOOST_TEST(film::sigmas(film::mean(rendered(scene), { kMiddleFrom, kMiddleFrom }, { kMiddleTo, kMiddleTo }), 1.0F / 6.0F) < 3.0F);
    }
    {
        // a prolate spheroid of semi-axes a = 0.5 and c = 1 has an area of
        // 2 pi a^2 (1 + c asin(e) / (a e))
        scene::Scene scene(scene::kinds());
        seen(scene, "sphereLight", glm::scale(glm::mat4(1.0F), glm::vec3 { 1.0F, 1.0F, 2.0F }));
        const float eccentricity = std::sqrt(1.0F - 0.25F);
        const float area = 2.0F * kPi * 0.25F * (1.0F + (std::asin(eccentricity) / (0.5F * eccentricity)));
        BOOST_TEST(film::sigmas(film::mean(rendered(scene), { kMiddleFrom, kMiddleFrom }, { kMiddleTo, kMiddleTo }), 1.0F / area) < 3.0F);
    }
    {
        scene::Scene scene(scene::kinds());
        const scene::Handle cylinder = seen(scene, "cylinderLight", glm::mat4(1.0F));
        scene.set(cylinder, "radius", scene::Value::number(0.25F));
        scene.set(cylinder, "length", scene::Value::number(2.0F));
        const film::Snapshot snapshot = rendered(scene);
        BOOST_TEST(film::sigmas(film::mean(snapshot, { 26, 14 }, { 30, 18 }), 1.0F / kPi) < 3.0F);
        BOOST_TEST(film::sigmas(film::mean(snapshot, { 2, 14 }, { 6, 18 }), 1.0F / kPi) < 3.0F);
        BOOST_TEST(film::mean(snapshot, { 14, 26 }, { 18, 30 }).value == 0.0F);
    }
}

/** A white plane, viewed from five units back across a half width, lit by a strategy. **/
scene::Handle litPlane(scene::Scene & scene, const char * strategy, std::uint32_t samples, float half) {
    const scene::Handle options = viewed(scene, samples, 5.0F, glm::vec2 { 0.0F }, half);
    scene.set(options, "lightSampling", scene::Value::string(strategy));
    plane(scene);
    return options;
}

/**
 * The radiance a white diffuse plane sends back from a point a distance off the foot of a point of
 * a radiant intensity of one at a height: its irradiance over pi.
 **/
float underPoint(const glm::vec2 & at, float /*radius*/, float height) {
    const float squared = (height * height) + glm::dot(at, at);
    return height / (squared * std::sqrt(squared) * kPi);
}

/** A sphere light at a height above the plane, normalised to a radiant intensity of one. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the radius, then the height
scene::Handle bulb(scene::Scene & scene, float radius, float height) {
    const scene::Handle sphere = scene.create("sphereLight", "bulb");
    scene.set(sphere, "radius", scene::Value::number(radius));
    scene.set(sphere, "normalize", scene::Value::integer(1));
    scene.set(sphere, "intensity", scene::Value::number(4.0F));
    scene.set(sphere, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -height)));
    return sphere;
}

// a white plane under a distant light of no angle and an irradiance of one, at sixty degrees,
// sends back half of one over pi. Only a light sample finds it, so it takes the whole weight, and
// is found whatever the strategy
BOOST_AUTO_TEST_CASE(moya_lights_a_distant_light_of_no_angle_gives_its_closed_form_test) {
    for (const char * strategy : { "bsdf", "light", "mis" }) {
        BOOST_TEST_CONTEXT(strategy) {
            scene::Scene scene(scene::kinds());
            litPlane(scene, strategy, 16, 0.25F);
            const scene::Handle sun = scene.create("distantLight", "sun");
            scene.set(sun, "angle", scene::Value::number(0.0F));
            scene.set(sun, "transform", scene::Value::matrix(glm::rotate(flipped(glm::mat4(1.0F)), glm::radians(60.0F), glm::vec3 { 1.0F, 0.0F, 0.0F })));
            const film::Estimate lit = film::mean(rendered(scene), { 0, 0 }, { kSide, kSide });
            BOOST_TEST(std::fabs(lit.value - (0.5F / kPi)) < 2.0e-3F);
            BOOST_TEST(film::sigmas(lit, 0.5F / kPi) < 3.0F);
        }
    }
}

// a white plane under a distant light with an angle, normalised, sends back one over pi, by light
// samples alone, by BSDF samples alone and by both. With normalize left out, its radiance is its
// intensity, and the plane sends back sin^2 of its half angle
BOOST_AUTO_TEST_CASE(moya_lights_a_distant_light_with_an_angle_gives_its_closed_form_by_each_strategy_test) {
    const auto lit = [](const char * strategy, std::int32_t normalize) {
        scene::Scene scene(scene::kinds());
        litPlane(scene, strategy, 256, 0.25F);
        const scene::Handle sun = scene.create("distantLight", "sun");
        scene.set(sun, "angle", scene::Value::number(60.0F));
        scene.set(sun, "normalize", scene::Value::integer(normalize));
        scene.set(sun, "transform", scene::Value::matrix(flipped(glm::mat4(1.0F))));
        return film::mean(rendered(scene), { 0, 0 }, { kSide, kSide });
    };
    for (const char * strategy : { "bsdf", "light", "mis" }) {
        BOOST_TEST_CONTEXT(strategy) {
            const film::Estimate normalised = lit(strategy, 1);
            BOOST_TEST(film::sigmas(normalised, 1.0F / kPi) < 3.0F);
            BOOST_TEST(film::sigmas(normalised, 1.05F / kPi) > 3.0F);
        }
    }
    BOOST_TEST(film::sigmas(lit("mis", 0), 0.25F) < 3.0F);
}

// a distant light's angle softens a shadow's edge over the width the angle gives. A cover one unit
// above the plane ends at x = 0. Under a light of no angle, the plane is black on one side of the
// edge and lit whole on the other. Under one of twenty degrees, the shadow's edge spreads over
// tan(10) of a unit on each side, and is lit there by a share. A path makes one diffuse bounce, so
// the light the plane sends up to the cover's underside does not come back down into the shadow
BOOST_AUTO_TEST_CASE(moya_lights_a_distant_lights_angle_softens_a_shadow_test) {
    const auto lit = [](float angle) {
        scene::Scene scene(scene::kinds());
        scene.set(litPlane(scene, "mis", 64, 0.5F), "diffuseDepth", scene::Value::integer(1));
        const scene::Handle sun = scene.create("distantLight", "sun");
        scene.set(sun, "angle", scene::Value::number(angle));
        scene.set(sun, "normalize", scene::Value::integer(1));
        scene.set(sun, "transform", scene::Value::matrix(flipped(glm::mat4(1.0F))));
        const scene::Handle cover = scene.create("disk", "cover");
        scene.set(cover, "radius", scene::Value::number(100.0F));
        scene.set(instanced(scene, "cover", cover, {}, moved(-100.0F, 0.0F, -1.0F)), "camera", scene::Value::integer(0));
        return rendered(scene);
    };
    // each pixel is a 32nd of a unit wide, so the edge lies between columns 15 and 16, and the
    // softened one spreads from column 10 to 21. The pixel filter reaches a pixel past each
    const film::Snapshot sharp = lit(0.0F);
    const film::Snapshot soft = lit(20.0F);
    BOOST_TEST(film::mean(sharp, { 12, 0 }, { 14, kSide }).value == 0.0F);
    BOOST_TEST(film::sigmas(film::mean(sharp, { 18, 0 }, { 20, kSide }), 1.0F / kPi) < 3.0F);
    BOOST_TEST(film::mean(soft, { 0, 0 }, { 8, kSide }).value == 0.0F);
    BOOST_TEST(film::sigmas(film::mean(soft, { 24, 0 }, { kSide, kSide }), 1.0F / kPi) < 3.0F);
    BOOST_TEST(film::mean(soft, { 12, 0 }, { 14, kSide }).value > 0.05F / kPi);
    BOOST_TEST(film::mean(soft, { 18, 0 }, { 20, kSide }).value < 0.95F / kPi);
}

// a white plane under a point of a radiant intensity of one reaches the closed form of a point's
// irradiance, whatever the strategy, since only a light sample finds it
BOOST_AUTO_TEST_CASE(moya_lights_a_point_gives_its_closed_form_test) {
    const float expected = meanOver(glm::vec2 { 0.0F }, 0.5F, &underPoint, 0.0F, 1.0F);
    for (const char * strategy : { "bsdf", "light", "mis" }) {
        BOOST_TEST_CONTEXT(strategy) {
            scene::Scene scene(scene::kinds());
            litPlane(scene, strategy, 16, 0.5F);
            bulb(scene, 0.0F, 1.0F);
            const film::Snapshot snapshot = rendered(scene);
            BOOST_TEST(film::sigmas(film::mean(snapshot, { 0, 0 }, { kSide, kSide }), expected) < 3.0F);
            BOOST_TEST(film::sigmas(film::mean(snapshot, { 0, 0 }, { kSide, kSide }), expected * 1.05F) > 10.0F);
        }
    }
}

// a point is the sphere it stands for. A small normalised sphere light gives the point's picture,
// and so does a sphere treated as a point. Without normalize, a point's intensity is its
// radiance times pi times its radius squared, so one of no radius emits nothing
BOOST_AUTO_TEST_CASE(moya_lights_a_point_is_the_limit_of_a_shrinking_sphere_test) {
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the radius, then the two switches
    const auto lit = [](float radius, std::int32_t point, std::int32_t normalize) {
        scene::Scene scene(scene::kinds());
        litPlane(scene, "mis", 64, 0.5F);
        const scene::Handle sphere = bulb(scene, radius, 1.0F);
        scene.set(sphere, "treatAsPoint", scene::Value::integer(point));
        scene.set(sphere, "normalize", scene::Value::integer(normalize));
        return film::mean(rendered(scene), { 0, 0 }, { kSide, kSide });
    };
    const float expected = meanOver(glm::vec2 { 0.0F }, 0.5F, &underPoint, 0.0F, 1.0F);
    const film::Estimate small = lit(0.02F, 0, 1);
    BOOST_TEST(film::sigmas(small, expected) < 3.0F);
    BOOST_TEST(film::sigmas(small, lit(0.0F, 0, 1)) < 3.0F);
    BOOST_TEST(film::sigmas(lit(0.5F, 1, 1), expected) < 3.0F);
    BOOST_TEST(film::sigmas(lit(0.5F, 1, 0), expected * 4.0F * kPi * 0.25F) < 3.0F);
    BOOST_TEST(lit(0.0F, 0, 0).value == 0.0F);
}

// a point's cone shapes it as a shaped light's does: a point on the plane far off its axis sees it
// past a cone of ten degrees
BOOST_AUTO_TEST_CASE(moya_lights_a_point_outside_its_cone_lights_nothing_test) {
    const auto lit = [](float cone) {
        scene::Scene scene(scene::kinds());
        viewed(scene, 16, 5.0F, glm::vec2 { 2.5F, 0.0F }, 0.25F);
        plane(scene);
        const scene::Handle spot = bulb(scene, 0.0F, 1.0F);
        scene.set(spot, "transform", scene::Value::matrix(flipped(moved(0.0F, 0.0F, -1.0F))));
        scene.set(spot, "coneAngle", scene::Value::number(cone));
        return film::mean(rendered(scene), { 0, 0 }, { kSide, kSide });
    };
    BOOST_TEST(lit(10.0F).value == 0.0F);
    BOOST_TEST(lit(90.0F).value > 0.0F);
}

/** A geometry made black, so that it reflects nothing. **/
void black(scene::Scene & scene, const scene::Handle & geometry) {
    scene.declare(geometry, scene::Declaration { .name = "Cs",
        .type = scene::Type::Colour,
        .count = scene::Declaration::kAny,
        .fallback = scene::Value::colours({}),
        .interpolation = scene::Interpolation::Constant });
    scene.set(geometry, "Cs", scene::Value::colours({ glm::vec3 { 0.0F } }));
}

/**
 * A black square mesh of a half side about the origin in z = 0, which a mesh light makes emit. It
 * is three triangles wound to face +z, fanned from a point on its top edge. They hold a half, two
 * fifths and a tenth of its area, so a triangle drawn by anything but its area is drawn wrongly.
 **/
scene::Handle square(scene::Scene & scene, const std::string & name, float half) {
    const glm::vec3 lowLeft { -half, -half, 0.0F };
    const glm::vec3 lowRight { half, -half, 0.0F };
    const glm::vec3 highRight { half, half, 0.0F };
    const glm::vec3 highLeft { -half, half, 0.0F };
    const glm::vec3 fanned { -0.6F * half, half, 0.0F };
    const scene::Handle out = mesh(scene, name, { lowLeft, lowRight, fanned, lowRight, highRight, fanned, lowLeft, fanned, highLeft });
    black(scene, out);
    return out;
}

/** A mesh light of a name that makes a geometry emit. **/
scene::Handle glowing(scene::Scene & scene, const scene::Handle & geometry, const std::string & name = "glow") {
    const scene::Handle light = scene.create("meshLight", name);
    scene.set(light, "geometry", scene::Value::handle(geometry));
    return light;
}

/**
 * A white plane under a black square of a half side at a height, which a mesh light makes emit.
 * The camera looks through the square, as it looks through a rect light.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the half side, then the height
film::Snapshot underSquare(const char * strategy, float half, float height) {
    scene::Scene scene(scene::kinds());
    litPlane(scene, strategy, 256, 0.25F);
    const scene::Handle panel = square(scene, "panel", half);
    scene.set(instanced(scene, "panel", panel, {}, moved(0.0F, 0.0F, -height)), "camera", scene::Value::integer(0));
    glowing(scene, panel);
    return rendered(scene);
}

/** A white plane under a rect light of a side at a height, facing it. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the side, then the height
film::Snapshot underRect(float side, float height) {
    scene::Scene scene(scene::kinds());
    litPlane(scene, "mis", 256, 0.25F);
    const scene::Handle rect = scene.create("rectLight", "rect");
    scene.set(rect, "width", scene::Value::number(side));
    scene.set(rect, "height", scene::Value::number(side));
    scene.set(rect, "transform", scene::Value::matrix(flipped(moved(0.0F, 0.0F, -height))));
    return rendered(scene);
}

// a mesh light of two triangles in a square gives the picture of a rect light of that square, by
// light samples alone, by BSDF samples alone and by both
BOOST_AUTO_TEST_CASE(moya_lights_a_mesh_light_square_gives_a_rect_lights_picture_test) {
    const film::Snapshot rect = underRect(1.0F, 0.5F);
    const film::Estimate expected = film::mean(rect, { 0, 0 }, { kSide, kSide });
    for (const char * strategy : { "bsdf", "light", "mis" }) {
        BOOST_TEST_CONTEXT(strategy) {
            const film::Snapshot glowed = underSquare(strategy, 0.5F, 0.5F);
            BOOST_TEST(film::sigmas(film::mean(glowed, { 0, 0 }, { kSide, kSide }), expected) < 3.0F);
            BOOST_TEST(film::furthest(glowed, rect).sigmas < 5.0F);
        }
    }
    BOOST_TEST(expected.value > 0.1F);
}

// a mesh light emits wherever a chain of instances places its geometry. A square in a group, above
// the plane by its own instance, that two instances place either side of the middle lights it
// twice as much as one of them does, by light samples alone
BOOST_AUTO_TEST_CASE(moya_lights_a_mesh_light_instanced_twice_gives_twice_the_light_test) {
    const auto lit = [](bool both) {
        scene::Scene scene(scene::kinds());
        litPlane(scene, "light", 128, 0.25F);
        const scene::Handle group = scene.create("group", "pair");
        const scene::Handle panel = square(scene, "panel", 0.25F);
        instanced(scene, "panel", panel, group, moved(0.0F, 0.0F, -0.5F));
        glowing(scene, panel);
        instanced(scene, "right", group, {}, moved(0.6F, 0.0F, 0.0F));
        if (both) {
            instanced(scene, "left", group, {}, moved(-0.6F, 0.0F, 0.0F));
        }
        return film::mean(rendered(scene), { 0, 0 }, { kSide, kSide });
    };
    const film::Estimate one = lit(false);
    const film::Estimate two = lit(true);
    BOOST_TEST(one.value > 0.0F);
    BOOST_TEST(film::sigmas(two, film::Estimate { .value = 2.0F * one.value, .error = 2.0F * one.error }) < 3.0F);
}

// with normalize, a mesh light's radiance is divided by its area in the world. A square scaled to
// twice its side gives the picture of a normalised rect light of that size
BOOST_AUTO_TEST_CASE(moya_lights_a_normalised_mesh_light_is_divided_by_its_area_in_the_world_test) {
    scene::Scene scene(scene::kinds());
    litPlane(scene, "mis", 256, 0.25F);
    const scene::Handle panel = square(scene, "panel", 0.5F);
    scene.set(instanced(scene, "panel", panel, {}, glm::scale(moved(0.0F, 0.0F, -0.5F), glm::vec3 { 2.0F })), "camera", scene::Value::integer(0));
    scene.set(glowing(scene, panel), "normalize", scene::Value::integer(1));
    const film::Snapshot glowed = rendered(scene);

    scene::Scene other(scene::kinds());
    litPlane(other, "mis", 256, 0.25F);
    const scene::Handle rect = other.create("rectLight", "rect");
    other.set(rect, "width", scene::Value::number(2.0F));
    other.set(rect, "height", scene::Value::number(2.0F));
    other.set(rect, "normalize", scene::Value::integer(1));
    other.set(rect, "transform", scene::Value::matrix(flipped(moved(0.0F, 0.0F, -0.5F))));
    const film::Estimate expected = film::mean(rendered(other), { 0, 0 }, { kSide, kSide });
    BOOST_TEST(film::sigmas(film::mean(glowed, { 0, 0 }, { kSide, kSide }), expected) < 3.0F);
    BOOST_TEST(expected.value > 0.02F);
}

/** A material whose surface node is RI's matte, which an instance names. **/
void withMaterial(scene::Scene & scene, const scene::Handle & instance, const std::string & name) {
    const scene::Handle node = scene.create("shader", name);
    scene.set(node, "shader", scene::Value::string("matte"));
    scene.set(node, "language", scene::Value::string("ri"));
    const scene::Handle material = scene.create("material", name);
    scene.set(material, "surface", scene::Value::handle(node));
    scene.set(instance, "material", scene::Value::handle(material));
}

/**
 * A scene rendered with every material made an OpenPBR surface that reflects nothing and emits a
 * luminance in white. Each material runs no program, so its parameters are the ones set here.
 **/
film::Snapshot renderedGlowing(scene::Scene & scene, float luminance) {
    scene.commit();
    scene.apply();
    geometry::Prototypes prototypes = geometry::gather(scene, {});
    for (kernel::surface::Material & material : prototypes.materials) {
        material.layered = 1U;
        material.program = kernel::program::Program {};
        material.surface.baseColour = glm::vec3 { 0.0F };
        material.surface.specularWeight = 0.0F;
        material.surface.emissionLuminance = luminance;
    }
    CpuDevice device;
    return integrate::render(device, scene, prototypes, {}, colour::Config());
}

/**
 * A white plane under a square of a half side at a height whose material emits a luminance, and
 * which a mesh light also makes emit where one is asked for.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the strategy, luminance and light
film::Snapshot underGlowingSquare(const char * strategy, float luminance, bool meshLight) {
    scene::Scene scene(scene::kinds());
    litPlane(scene, strategy, 256, 0.25F);
    const scene::Handle panel = square(scene, "panel", 0.5F);
    const scene::Handle placed = instanced(scene, "panel", panel, {}, moved(0.0F, 0.0F, -0.5F));
    scene.set(placed, "camera", scene::Value::integer(0));
    withMaterial(scene, placed, "emissive");
    if (meshLight) {
        glowing(scene, panel);
    }
    return renderedGlowing(scene, luminance);
}

// a square whose material emits gives the picture of a rect light of its radiance, by light samples
// alone, by BSDF samples alone and by both, as a mesh light does
BOOST_AUTO_TEST_CASE(moya_lights_an_emitting_material_gives_a_rect_lights_picture_test) {
    const film::Snapshot rect = underRect(1.0F, 0.5F);
    const film::Estimate expected = film::mean(rect, { 0, 0 }, { kSide, kSide });
    for (const char * strategy : { "bsdf", "light", "mis" }) {
        BOOST_TEST_CONTEXT(strategy) {
            const film::Snapshot glowed = underGlowingSquare(strategy, 1.0F, false);
            BOOST_TEST(film::sigmas(film::mean(glowed, { 0, 0 }, { kSide, kSide }), expected) < 3.0F);
            BOOST_TEST(film::furthest(glowed, rect).sigmas < 5.0F);
        }
    }
}

// a square whose material emits a luminance shows that luminance to the camera, in a black scene
BOOST_AUTO_TEST_CASE(moya_lights_an_emitting_material_shows_its_luminance_test) {
    scene::Scene scene(scene::kinds());
    viewed(scene, 16, 5.0F, glm::vec2 { 0.0F }, 0.25F);
    const scene::Handle panel = square(scene, "panel", 1.0F);
    withMaterial(scene, instanced(scene, "panel", panel, {}, flipped(moved(0.0F, 0.0F, -0.5F))), "emissive");
    const film::Estimate seen = film::mean(renderedGlowing(scene, 2.5F), { 0, 0 }, { kSide, kSide });
    BOOST_TEST(seen.value == 2.5F, boost::test_tools::tolerance(1.0e-3F));
}

// a mesh light and an emitting material on one geometry both emit, and their light adds: a square
// of both lights the plane as a rect light of their summed radiance does
BOOST_AUTO_TEST_CASE(moya_lights_a_mesh_light_and_an_emitting_material_add_test) {
    const film::Estimate alone = film::mean(underRect(1.0F, 0.5F), { 0, 0 }, { kSide, kSide });
    const film::Estimate both = film::mean(underGlowingSquare("mis", 1.5F, true), { 0, 0 }, { kSide, kSide });
    BOOST_TEST(film::sigmas(both, film::Estimate { .value = 2.5F * alone.value, .error = 2.5F * alone.error }) < 3.0F);
}

// a moving mesh light lights the places it passes. A square that crosses the plane over the
// shutter lights the far end of its path, which it lights little from where it starts. Light
// samples and BSDF samples give the same light over the picture and along the path
BOOST_AUTO_TEST_CASE(moya_lights_a_moving_mesh_light_lights_the_places_it_passes_test) {
    const auto lit = [](const char * strategy, bool moves) {
        scene::Scene scene(scene::kinds());
        const scene::Handle options = viewed(scene, 256, 5.0F, glm::vec2 { 0.0F }, 1.5F);
        scene.commit();
        scene.apply();
        scene.set(scene.value(options, "camera").handleArray().front(), "shutter", scene::Value::floats({ 0.0F, 1.0F }));
        scene.set(options, "lightSampling", scene::Value::string(strategy));
        plane(scene);
        const scene::Handle panel = square(scene, "panel", 0.25F);
        const glm::mat4 start = moved(-1.0F, 0.0F, -0.3F);
        scene.set(instanced(scene, "panel", panel, {}, { 0.0F, 1.0F }, { start, moves ? moved(1.0F, 0.0F, -0.3F) : start }), "camera",
            scene::Value::integer(0));
        glowing(scene, panel);
        return rendered(scene);
    };
    const film::Snapshot light = lit("light", true);
    const film::Snapshot bsdf = lit("bsdf", true);
    BOOST_TEST(film::sigmas(film::mean(light, { 0, 0 }, { kSide, kSide }), film::mean(bsdf, { 0, 0 }, { kSide, kSide })) < 3.0F);
    // along the path alone, since BSDF samples rarely find the light from a pixel far from it
    BOOST_TEST(film::sigmas(film::mean(light, { 0, 12 }, { kSide, 20 }), film::mean(bsdf, { 0, 12 }, { kSide, 20 })) < 3.0F);
    // the far end of the path is the right sixth of the picture
    const float passed = film::mean(light, { 26, 0 }, { kSide, kSide }).value;
    BOOST_TEST(passed > 3.0F * film::mean(lit("light", false), { 26, 0 }, { kSide, kSide }).value);
}

// a subdivision surface emits from the triangles the gather cuts it into. A bilinear quad is the
// flat square of its corners, so it gives the picture of a rect light of that square. The camera
// looks through each
BOOST_AUTO_TEST_CASE(moya_lights_a_subdivision_surface_mesh_light_emits_from_its_triangles_test) {
    scene::Scene scene(scene::kinds());
    litPlane(scene, "mis", 256, 0.25F);
    const scene::Handle cage = scene.create("subdivision", "cage");
    scene.set(cage, "scheme", scene::Value::string("bilinear"));
    scene.set(cage, "P",
        scene::Value::points({ glm::vec3 { -0.5F, -0.5F, 0.0F }, glm::vec3 { 0.5F, -0.5F, 0.0F }, glm::vec3 { 0.5F, 0.5F, 0.0F }, glm::vec3 { -0.5F, 0.5F, 0.0F } }));
    scene.set(cage, "vertices", scene::Value::integers({ 4 }));
    scene.set(cage, "indices", scene::Value::integers({ 0, 1, 2, 3 }));
    black(scene, cage);
    scene.set(instanced(scene, "cage", cage, {}, moved(0.0F, 0.0F, -0.5F)), "camera", scene::Value::integer(0));
    glowing(scene, cage);
    const film::Snapshot glowed = rendered(scene);
    const film::Snapshot rect = underRect(1.0F, 0.5F);
    BOOST_TEST(film::sigmas(film::mean(glowed, { 0, 0 }, { kSide, kSide }), film::mean(rect, { 0, 0 }, { kSide, kSide })) < 3.0F);
}

// camera rays see what a mesh light emits only while its camera is 1. The square faces the camera,
// which sees its radiance of one there, and its black surface otherwise
BOOST_AUTO_TEST_CASE(moya_lights_camera_rays_see_a_mesh_light_only_when_set_test) {
    const auto seenAt = [](std::int32_t camera) {
        scene::Scene scene(scene::kinds());
        viewed(scene, 16, 5.0F, glm::vec2 { 0.0F }, 0.25F);
        const scene::Handle panel = square(scene, "panel", 1.0F);
        instanced(scene, "panel", panel, {}, flipped(glm::mat4(1.0F)));
        scene.set(glowing(scene, panel), "camera", scene::Value::integer(camera));
        return film::mean(rendered(scene), { 0, 0 }, { kSide, kSide });
    };
    BOOST_TEST(film::sigmas(seenAt(1), 1.0F) < 3.0F);
    BOOST_TEST(seenAt(0).value == 0.0F);
}

// a mesh light emits from the side its triangles' winding faces: a square facing the plane lights
// it, and one turned away lights nothing, by any strategy. The camera looks through it
BOOST_AUTO_TEST_CASE(moya_lights_a_mesh_light_emits_from_one_side_test) {
    const auto lit = [](const char * strategy, bool away) {
        scene::Scene scene(scene::kinds());
        litPlane(scene, strategy, 16, 0.25F);
        const scene::Handle panel = square(scene, "panel", 0.5F);
        const glm::mat4 place = moved(0.0F, 0.0F, -0.5F);
        scene.set(instanced(scene, "panel", panel, {}, away ? flipped(place) : place), "camera", scene::Value::integer(0));
        glowing(scene, panel);
        return film::mean(rendered(scene), { 0, 0 }, { kSide, kSide }).value;
    };
    for (const char * strategy : { "bsdf", "light", "mis" }) {
        BOOST_TEST_CONTEXT(strategy) {
            BOOST_TEST(lit(strategy, false) > 0.1F);
            BOOST_TEST(lit(strategy, true) == 0.0F);
        }
    }
}

// a mesh light of a geometry with no triangles emits nothing: a sphere a mesh light names adds
// nothing to the light of a square another mesh light makes emit
BOOST_AUTO_TEST_CASE(moya_lights_a_mesh_light_of_no_triangles_emits_nothing_test) {
    const auto lit = [](bool ball) {
        scene::Scene scene(scene::kinds());
        litPlane(scene, "light", 16, 0.25F);
        const scene::Handle panel = square(scene, "panel", 0.5F);
        scene.set(instanced(scene, "panel", panel, {}, moved(0.0F, 0.0F, -0.5F)), "camera", scene::Value::integer(0));
        glowing(scene, panel);
        if (ball) {
            const scene::Handle sphere = scene.create("sphere", "ball");
            instanced(scene, "ball", sphere, {}, moved(3.0F, 0.0F, -2.0F));
            glowing(scene, sphere, "glow ball");
        }
        return film::mean(rendered(scene), { 0, 0 }, { kSide, kSide });
    };
    const film::Estimate alone = lit(false);
    BOOST_TEST(alone.value > 0.1F);
    BOOST_TEST(film::sigmas(lit(true), alone) < 3.0F);
}

/**
 * An EXR image of texels, row after row from its top, written to a file of a name in the
 * temporary directory, which says it is a cube map when asked. Gives the file's path.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the width, then the height
std::string imageFile(const std::string & name, std::uint32_t width, std::uint32_t height, const std::vector<glm::vec3> & texels, bool cube = false) {
    const std::string path = (boost::filesystem::temp_directory_path() / ("v3d_lights_" + name + ".exr")).string();
    Imf::Header header(static_cast<int>(width), static_cast<int>(height));
    if (cube) {
        Imf::addEnvmap(header, Imf::ENVMAP_CUBE);
    }
    Imf::RgbaOutputFile file(path.c_str(), header, Imf::WRITE_RGB);
    std::vector<Imf::Rgba> pixels;
    pixels.reserve(texels.size());
    for (const glm::vec3 & texel : texels) {
        pixels.emplace_back(texel.x, texel.y, texel.z, 1.0F);
    }
    file.setFrameBuffer(pixels.data(), 1, width);
    file.writePixels(static_cast<int>(height));
    return path;
}

/** A dome light that reads an image file, or none when the path is empty. **/
scene::Handle sky(scene::Scene & scene, const std::string & path) {
    const scene::Handle dome = scene.create("domeLight", "sky");
    scene.set(dome, "texture", scene::Value::string(path));
    return dome;
}

/** The image of a dome of a width and a height, black but for one texel. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the size, then the texel
std::vector<glm::vec3> oneTexel(const glm::uvec2 & size, const glm::uvec2 & lit, float value) {
    std::vector<glm::vec3> out(static_cast<std::size_t>(size.x) * size.y, glm::vec3 { 0.0F });
    out[(static_cast<std::size_t>(lit.y) * size.x) + lit.x] = glm::vec3 { value };
    return out;
}

// a dome's image is laid out as OpenEXR lays out latitude and longitude: its middle faces +z, a
// quarter of the way in from its left +x, and its top +y. A direction and its place in the image
// are each other's
BOOST_AUTO_TEST_CASE(moya_lights_a_domes_image_faces_as_usdlux_lays_it_out_test) {
    BOOST_TEST(glm::distance(kernel::light::domeDirection(glm::vec2 { 0.5F, 0.5F }), glm::vec3 { 0.0F, 0.0F, 1.0F }) < 1.0e-6F);
    BOOST_TEST(glm::distance(kernel::light::domeDirection(glm::vec2 { 0.25F, 0.5F }), glm::vec3 { 1.0F, 0.0F, 0.0F }) < 1.0e-6F);
    BOOST_TEST(glm::distance(kernel::light::domeDirection(glm::vec2 { 0.75F, 0.5F }), glm::vec3 { -1.0F, 0.0F, 0.0F }) < 1.0e-6F);
    BOOST_TEST(glm::distance(kernel::light::domeDirection(glm::vec2 { 0.5F, 0.0F }), glm::vec3 { 0.0F, 1.0F, 0.0F }) < 1.0e-6F);
    for (const glm::vec2 at : { glm::vec2 { 0.1F, 0.4F }, glm::vec2 { 0.7F, 0.9F }, glm::vec2 { 0.45F, 0.2F } }) {
        BOOST_TEST(glm::distance(kernel::light::domePlace(kernel::light::domeDirection(at)), at) < 1.0e-5F);
    }
}

// a white plane under a dome of an image of one everywhere sends back one, by light samples
// alone, by BSDF samples alone and by both
BOOST_AUTO_TEST_CASE(moya_lights_a_dome_of_one_colour_gives_the_furnace_test) {
    const std::string path = imageFile("even", 16, 8, std::vector<glm::vec3>(std::size_t { 128 }, glm::vec3 { 1.0F }));
    for (const char * strategy : { "bsdf", "light", "mis" }) {
        BOOST_TEST_CONTEXT(strategy) {
            scene::Scene scene(scene::kinds());
            litPlane(scene, strategy, 64, 0.25F);
            sky(scene, path);
            BOOST_TEST(film::sigmas(film::mean(rendered(scene), { 0, 0 }, { kSide, kSide }), 1.0F) < 3.0F);
        }
    }
}

/**
 * The radiance a white diffuse plane facing -z sends back under one texel of a dome of a value,
 * turned into the world. It is the value times the integral over the texel of the cosine to the
 * plane's normal, over pi.
 **/
float underTexel(float value, const glm::mat4 & turn) {
    constexpr std::uint32_t kSteps = 64;
    double sum = 0.0;
    const double cell = 1.0 / (static_cast<double>(kDomeWidth) * kSteps * kDomeHeight * kSteps);
    for (std::uint32_t row = 0; row < kSteps; ++row) {
        for (std::uint32_t column = 0; column < kSteps; ++column) {
            const glm::vec2 at { (static_cast<float>(kLitColumn) + ((static_cast<float>(column) + 0.5F) / kSteps)) / kDomeWidth,
                (static_cast<float>(kLitRow) + ((static_cast<float>(row) + 0.5F) / kSteps)) / kDomeHeight };
            const glm::vec3 local = kernel::light::domeDirection(at);
            const glm::vec3 towards = glm::mat3(turn) * local;
            const double across = std::sqrt(1.0 - (static_cast<double>(local.y) * local.y));
            sum += std::fmax(0.0, -static_cast<double>(towards.z)) * kernel::light::kTwicePiSquared * across * cell;
        }
    }
    return static_cast<float>(value * sum / kPi);
}

// a dome of one bright texel lights a plane as its closed form says, by light samples alone and by
// both, and as a distant light from the texel's middle does. Turned a quarter about y, it lights
// the plane as the turned closed form says. Turned half about y, the texel lies behind the plane,
// and lights nothing
BOOST_AUTO_TEST_CASE(moya_lights_a_dome_of_one_bright_texel_lights_as_a_distant_light_test) {
    constexpr float kBright = 500.0F;
    const std::string path = imageFile("texel", kDomeWidth, kDomeHeight, oneTexel({ kDomeWidth, kDomeHeight }, { kLitColumn, kLitRow }, kBright));
    const auto lit = [&](const char * strategy, const glm::mat4 & turn) {
        scene::Scene scene(scene::kinds());
        litPlane(scene, strategy, 64, 0.25F);
        scene.set(sky(scene, path), "transform", scene::Value::matrix(turn));
        return film::mean(rendered(scene), { 0, 0 }, { kSide, kSide });
    };
    const glm::mat4 quarter = glm::rotate(glm::mat4(1.0F), glm::radians(90.0F), glm::vec3 { 0.0F, 1.0F, 0.0F });
    const float expected = underTexel(kBright, glm::mat4(1.0F));
    for (const char * strategy : { "light", "mis" }) {
        BOOST_TEST_CONTEXT(strategy) {
            BOOST_TEST(film::sigmas(lit(strategy, glm::mat4(1.0F)), expected) < 3.0F);
            BOOST_TEST(film::sigmas(lit(strategy, quarter), underTexel(kBright, quarter)) < 3.0F);
        }
    }
    BOOST_TEST(std::fabs(underTexel(kBright, quarter) - expected) > 0.1F * expected);
    BOOST_TEST(lit("light", glm::rotate(glm::mat4(1.0F), glm::radians(180.0F), glm::vec3 { 0.0F, 1.0F, 0.0F })).value == 0.0F);

    // the texel's solid angle and middle, which a distant light of its irradiance shines from
    const float band = std::sin(kPi * (0.5F - (static_cast<float>(kLitRow) / kDomeHeight))) - std::sin(kPi * (0.5F - (static_cast<float>(kLitRow + 1U) / kDomeHeight)));
    const float angle = 2.0F * kPi * band / kDomeWidth;
    const glm::vec3 middle = kernel::light::domeDirection(
        glm::vec2 { (static_cast<float>(kLitColumn) + 0.5F) / kDomeWidth, (static_cast<float>(kLitRow) + 0.5F) / kDomeHeight });
    scene::Scene scene(scene::kinds());
    litPlane(scene, "light", 16, 0.25F);
    const scene::Handle sun = scene.create("distantLight", "sun");
    scene.set(sun, "angle", scene::Value::number(0.0F));
    scene.set(sun, "intensity", scene::Value::number(kBright * angle));
    scene.set(sun, "transform", scene::Value::matrix(scene::aimed(glm::vec3 { 0.0F }, -middle)));
    const float distant = film::mean(rendered(scene), { 0, 0 }, { kSide, kSide }).value;
    BOOST_TEST(std::fabs((distant / expected) - 1.0F) < 0.01F);
}

// light samples, BSDF samples and both give the same picture of a plane under a varied image: a
// gradient from the top down, with a brighter patch facing the plane
BOOST_AUTO_TEST_CASE(moya_lights_each_strategy_gives_one_picture_of_a_varied_dome_test) {
    std::vector<glm::vec3> texels;
    for (std::uint32_t row = 0; row < 16U; ++row) {
        for (std::uint32_t column = 0; column < 32U; ++column) {
            const bool patch = column >= 2U && column < 6U && row >= 5U && row < 9U;
            texels.push_back(glm::vec3 { 0.2F + (0.05F * static_cast<float>(row)), 0.3F, 0.1F } + glm::vec3 { patch ? 20.0F : 0.0F });
        }
    }
    const std::string path = imageFile("varied", 32, 16, texels);
    const auto lit = [&](const char * strategy) {
        scene::Scene scene(scene::kinds());
        litPlane(scene, strategy, 256, 0.25F);
        sky(scene, path);
        return film::mean(rendered(scene), { 0, 0 }, { kSide, kSide });
    };
    const film::Estimate both = lit("mis");
    BOOST_TEST(film::sigmas(lit("light"), both) < 3.0F);
    BOOST_TEST(film::sigmas(lit("bsdf"), both) < 3.0F);
    BOOST_TEST(both.value > 0.5F);
}

// a dome reads its image in latitude and longitude. A format of another layout, or a file that
// says it is a cube map, lights nothing, though latitude and longitude light the same image
BOOST_AUTO_TEST_CASE(moya_lights_a_dome_of_another_layout_emits_nothing_test) {
    const std::vector<glm::vec3> even(std::size_t { 128 }, glm::vec3 { 1.0F });
    const auto lit = [](const std::string & path, const char * format) {
        scene::Scene scene(scene::kinds());
        litPlane(scene, "mis", 16, 0.25F);
        scene.set(sky(scene, path), "format", scene::Value::string(format));
        return film::mean(rendered(scene), { 0, 0 }, { kSide, kSide }).value;
    };
    const std::string plain = imageFile("plain", 16, 8, even);
    const std::string cube = imageFile("cube", 16, 8, even, true);
    BOOST_TEST(lit(plain, "latlong") > 0.5F);
    BOOST_TEST(lit(plain, "automatic") > 0.5F);
    BOOST_TEST(lit(plain, "angular") == 0.0F);
    BOOST_TEST(lit(cube, "automatic") == 0.0F);
}

// a rect light's texture tints what it emits. Its first corner lies at the rect's +x and +y. A
// texture white on its left half and black on its right then gives the picture of a rect of half
// the width on the +x side
BOOST_AUTO_TEST_CASE(moya_lights_a_rect_lights_texture_tints_what_it_emits_test) {
    const std::string path = imageFile("halves", 2, 1, { glm::vec3 { 1.0F }, glm::vec3 { 0.0F } });
    const auto lit = [&](bool textured) {
        scene::Scene scene(scene::kinds());
        litPlane(scene, "mis", 256, 0.25F);
        const scene::Handle rect = scene.create("rectLight", "rect");
        scene.set(rect, "width", scene::Value::number(textured ? 1.0F : 0.5F));
        scene.set(rect, "height", scene::Value::number(1.0F));
        scene.set(rect, "transform", scene::Value::matrix(flipped(moved(textured ? 0.0F : 0.25F, 0.0F, -0.5F))));
        if (textured) {
            scene.set(rect, "texture", scene::Value::string(path));
        }
        return rendered(scene);
    };
    const film::Snapshot textured = lit(true);
    const film::Snapshot half = lit(false);
    BOOST_TEST(film::sigmas(film::mean(textured, { 0, 0 }, { kSide, kSide }), film::mean(half, { 0, 0 }, { kSide, kSide })) < 3.0F);
    BOOST_TEST(film::furthest(textured, half).sigmas < 5.0F);
}

// each light with a shape is an instance in the world, after every other, with the light's index
// in the scene. It is visible to indirect rays, to camera rays when set, and never to shadow rays.
// A distant light and a point have no shape
BOOST_AUTO_TEST_CASE(moya_lights_each_shaped_light_is_an_instance_in_the_world_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle sphere = scene.create("sphereLight", "a");
    const scene::Handle cylinder = scene.create("cylinderLight", "b");
    scene.set(cylinder, "camera", scene::Value::integer(1));
    scene.create("distantLight", "sun");
    scene.set(scene.create("sphereLight", "point"), "radius", scene::Value::number(0.0F));
    scene.set(scene.create("sphereLight", "treated"), "treatAsPoint", scene::Value::integer(1));
    const scene::Handle ball = scene.create("sphere", "ball");
    instanced(scene, "ball", ball, {}, glm::mat4(1.0F));
    scene.commit();
    scene.apply();
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    // a distant light and the sphere lights of no size place no shape
    BOOST_TEST_REQUIRE(prototypes.lights.size() == 2U);
    BOOST_TEST((prototypes.lights[0].handle == sphere));
    BOOST_TEST((prototypes.lights[1].handle == cylinder));
    BOOST_TEST((prototypes.world.instances == std::vector<std::uint32_t> { 0U, 1U, 2U }));
    const geometry::Instance & first = prototypes.instances[prototypes.lights[0].instance];
    const geometry::Instance & second = prototypes.instances[prototypes.lights[1].instance];
    BOOST_TEST(first.id == sphere.index);
    BOOST_TEST(first.visibility == kernel::geometry::kIndirectRay);
    BOOST_TEST(second.visibility == (kernel::geometry::kIndirectRay | kernel::geometry::kCameraRay));
    // the cylinder's quadric lies along z, and is turned onto the light's x
    BOOST_TEST(glm::distance(glm::vec3(second.transform * glm::vec4 { 0.0F, 0.0F, 1.0F, 0.0F }), glm::vec3 { 1.0F, 0.0F, 0.0F }) < 1.0e-6F);
}

// a moving cylinder light's shape lies along the light's x at every time, as its keys turn it
BOOST_AUTO_TEST_CASE(moya_lights_a_moving_cylinder_light_lies_along_its_x_test) {
    scene::Scene scene(scene::kinds());
    shutter(scene);
    const scene::Handle cylinder = scene.create("cylinderLight", "tube");
    const glm::mat4 from = glm::scale(glm::mat4(1.0F), glm::vec3 { 1.0F, 2.0F, 3.0F });
    const glm::mat4 to = glm::rotate(moved(1.0F, 0.0F, 0.0F), glm::radians(60.0F), glm::vec3 { 0.0F, 0.0F, 1.0F }) * from;
    scene.set(cylinder, "transform", { 0.0F, 1.0F }, { scene::Value::matrix(from), scene::Value::matrix(to) });
    scene.commit();
    scene.apply();
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST_REQUIRE(prototypes.lights.size() == 1U);
    const geometry::Instance & instance = prototypes.instances[prototypes.lights[0].instance];
    BOOST_TEST_REQUIRE(instance.moves());
    for (const float time : { 0.0F, 1.0F }) {
        const glm::mat4 light = time == 0.0F ? from : to;
        const glm::vec3 along { geometry::transformAt(instance, time) * glm::vec4 { 0.0F, 0.0F, 1.0F, 0.0F } };
        BOOST_TEST(glm::distance(along, glm::vec3(light * glm::vec4 { 1.0F, 0.0F, 0.0F, 0.0F })) < 1.0e-5F);
    }
}

// changing a light builds its shape's hierarchy and the world's again, and no other
BOOST_AUTO_TEST_CASE(moya_lights_changing_a_light_rebuilds_its_shape_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle disk = scene.create("diskLight", "key");
    const scene::Handle ball = scene.create("sphere", "ball");
    instanced(scene, "ball", ball, {}, glm::mat4(1.0F));
    scene.commit();
    scene.apply();
    CpuDevice device;
    v3d::moya::accel::Assembler assembler(device, {});
    assembler.update(scene);
    scene.set(disk, "radius", scene::Value::number(2.0F));
    scene.commit();
    scene.apply();
    assembler.update(scene);
    BOOST_TEST((assembler.builtGeometries() == std::vector<std::uint32_t> { disk.index }));
    BOOST_TEST((assembler.builtGroups() == std::vector<std::uint32_t> { geometry::Group::kWorld }));
}

/**
 * A white plane under a light of every kind, viewed across a half width of 1.5, with samples per
 * pixel. A path finds the lights by a strategy, and a light sample draws one by a way of picking.
 * The shutter opens at 0 and closes at 1.
 *
 * The lights are a tilted rect light, a disk, a sphere and a cylinder light, a point, and a spot
 * with a soft cone. A rect light turns over the shutter, and a group places a square mesh light
 * twice, once mirrored. While afar holds, a distant light with an angle and a dome light join
 * them.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the strategy, then the picking
void everyKind(scene::Scene & scene, const char * strategy, const char * picking, std::uint32_t samples, bool afar) {
    const scene::Handle options = litPlane(scene, strategy, samples, 1.5F);
    scene.commit();
    scene.apply();
    scene.set(scene.value(options, "camera").handleArray().front(), "shutter", scene::Value::floats({ 0.0F, 1.0F }));
    scene.set(options, "lightPicking", scene::Value::string(picking));
    const scene::Handle rect = scene.create("rectLight", "rect");
    scene.set(rect, "transform", scene::Value::matrix(glm::rotate(moved(-0.8F, 0.6F, -1.0F), glm::radians(160.0F), glm::vec3 { 1.0F, 0.2F, 0.0F })));
    diskAbove(scene, 0.3F, 1.2F);
    const scene::Handle sphere = scene.create("sphereLight", "sphere");
    scene.set(sphere, "radius", scene::Value::number(0.2F));
    scene.set(sphere, "transform", scene::Value::matrix(moved(0.8F, 0.6F, -0.8F)));
    const scene::Handle cylinder = scene.create("cylinderLight", "cylinder");
    scene.set(cylinder, "radius", scene::Value::number(0.1F));
    scene.set(cylinder, "transform", scene::Value::matrix(moved(0.0F, -0.8F, -0.6F)));
    scene.set(bulb(scene, 0.0F, 0.7F), "transform", scene::Value::matrix(moved(-0.6F, -0.6F, -0.7F)));
    const scene::Handle spot = scene.create("sphereLight", "spot");
    scene.set(spot, "radius", scene::Value::number(0.0F));
    scene.set(spot, "normalize", scene::Value::integer(1));
    scene.set(spot, "intensity", scene::Value::number(4.0F));
    scene.set(spot, "coneAngle", scene::Value::number(25.0F));
    scene.set(spot, "coneSoftness", scene::Value::number(0.4F));
    scene.set(spot, "transform", scene::Value::matrix(flipped(moved(0.6F, -0.5F, -1.0F))));
    const scene::Handle turning = scene.create("rectLight", "turning");
    const glm::mat4 from = flipped(moved(-1.0F, -1.0F, -0.8F));
    scene.set(turning, "transform", { 0.0F, 1.0F }, { scene::Value::matrix(from), scene::Value::matrix(glm::rotate(from, glm::radians(40.0F), glm::vec3 { 1.0F, 0.0F, 0.0F })) });
    const scene::Handle group = scene.create("group", "pair");
    const scene::Handle panel = square(scene, "panel", 0.2F);
    scene.set(instanced(scene, "panel", panel, group, moved(0.0F, 0.0F, -0.5F)), "camera", scene::Value::integer(0));
    glowing(scene, panel);
    instanced(scene, "right", group, {}, moved(0.9F, 0.9F, 0.0F));
    instanced(scene, "left", group, {}, glm::scale(moved(-0.9F, 0.9F, 0.0F), glm::vec3 { -1.0F, 1.0F, 1.0F }));
    if (!afar) {
        return;
    }
    const scene::Handle sun = scene.create("distantLight", "sun");
    scene.set(sun, "angle", scene::Value::number(10.0F));
    scene.set(sun, "intensity", scene::Value::number(0.5F));
    scene.set(sun, "transform", scene::Value::matrix(glm::rotate(flipped(glm::mat4(1.0F)), glm::radians(20.0F), glm::vec3 { 0.0F, 1.0F, 0.0F })));
    scene.set(sky(scene, ""), "intensity", scene::Value::number(0.1F));
}

// the light tree gives the picture that drawing each light in proportion to its power gives, for
// a light of every kind with a place, by light samples alone and by both. Light samples alone show
// that no bound it holds leaves out a light that reaches a shading point, since BSDF samples would
// find such a light under multiple importance sampling. Both show that a BSDF sample that meets a
// light weighs it by the chance the tree draws it with. The distant and dome lights are left out,
// since their power over the world's reach leaves drawing by power too few draws of the others to
// compare with
BOOST_AUTO_TEST_CASE(moya_lights_the_light_tree_gives_the_picture_drawing_by_power_gives_test) {
    const auto lit = [](const char * strategy, const char * picking) {
        scene::Scene scene(scene::kinds());
        everyKind(scene, strategy, picking, 256, false);
        return rendered(scene);
    };
    for (const char * strategy : { "light", "mis" }) {
        BOOST_TEST_CONTEXT(strategy) {
            const film::Snapshot tree = lit(strategy, "tree");
            const film::Snapshot power = lit(strategy, "power");
            BOOST_TEST(film::sigmas(film::mean(tree, { 0, 0 }, { kSide, kSide }), film::mean(power, { 0, 0 }, { kSide, kSide })) < 3.0F);
            BOOST_TEST(film::furthest(tree, power).sigmas < 5.0F);
        }
    }
}

// with no light that has a place there is no tree, and the distant and dome lights are each drawn
// with an even share of the random numbers, and never nothing
BOOST_AUTO_TEST_CASE(moya_lights_the_light_tree_draws_lights_afar_alone_by_their_shares_test) {
    scene::Scene scene(scene::kinds());
    litPlane(scene, "mis", 1, 1.5F);
    const scene::Handle sun = scene.create("distantLight", "sun");
    scene.set(sun, "angle", scene::Value::number(10.0F));
    sky(scene, "");
    scene.commit();
    scene.apply();
    const Built built(scene, 0);
    const kernel::light::Lights lights = built.lights.view();
    const kernel::surface::Surfaces surfaces = built.surfaces.view();
    BOOST_TEST_REQUIRE(lights.rootOf(0) == kernel::geometry::kNoPrimitive);
    BOOST_TEST_REQUIRE(lights.afar(0) == 2U);
    constexpr std::uint32_t kDraws = 1024;
    std::map<std::uint32_t, std::uint32_t> counts;
    for (std::uint32_t draw = 0; draw < kDraws; ++draw) {
        const float u = (static_cast<float>(draw) + 0.5F) / static_cast<float>(kDraws);
        const kernel::light::Picked picked = lights.pickByTree(surfaces, 0, glm::vec3 { 0.0F }, glm::vec3 { 0.0F, 0.0F, -1.0F }, 0.0F, u);
        BOOST_TEST_REQUIRE(picked.place != kernel::geometry::kNoPrimitive);
        BOOST_TEST(picked.chance == 0.5F);
        ++counts[picked.place];
    }
    for (std::uint32_t i = 0; i < lights.afar(0); ++i) {
        const std::uint32_t place = lights.infinite[lights.infiniteFirst[0] + i];
        BOOST_TEST(counts[place] == kDraws / 2U);
        BOOST_TEST(lights.chanceByTree(surfaces, 0, place, kernel::geometry::kNoPrimitive, glm::vec3 { 0.0F }, glm::vec3 { 0.0F, 0.0F, -1.0F }, 0.0F) == 0.5F);
    }
}

// the light tree draws each light, and each triangle of a mesh light, with the chance it reports
// for it. Evenly spread random numbers draw each as often as its chance says, to within one at
// each end of its share of them, and draw nothing as often as the chances fall short of one
BOOST_AUTO_TEST_CASE(moya_lights_the_light_tree_draws_by_the_chance_it_reports_test) {
    scene::Scene scene(scene::kinds());
    everyKind(scene, "mis", "tree", 1, true);
    scene.commit();
    scene.apply();
    const Built built(scene, 0);
    const kernel::light::Lights lights = built.lights.view();
    const kernel::surface::Surfaces surfaces = built.surfaces.view();
    BOOST_TEST_REQUIRE(lights.rootOf(0) != kernel::geometry::kNoPrimitive);
    constexpr std::uint32_t kDraws = 1U << 16U;
    const std::vector<std::pair<glm::vec3, glm::vec3>> shading { { glm::vec3 { 0.0F }, glm::vec3 { 0.0F, 0.0F, -1.0F } },
        { glm::vec3 { 1.2F, -0.4F, 0.0F }, glm::vec3 { 0.0F, 0.0F, -1.0F } }, { glm::vec3 { 0.1F, 0.2F, -0.6F }, glm::vec3 { 0.6F, 0.0F, -0.8F } },
        { glm::vec3 { -0.5F, 0.5F, -0.3F }, glm::vec3 { 0.0F } } };
    for (const auto & [point, normal] : shading) {
        BOOST_TEST_CONTEXT("at " << point.x << " " << point.y << " " << point.z) {
            std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> counts;
            std::uint32_t none = 0;
            float furthest = 0.0F;
            for (std::uint32_t draw = 0; draw < kDraws; ++draw) {
                const float u = (static_cast<float>(draw) + 0.5F) / static_cast<float>(kDraws);
                const kernel::light::Picked picked = lights.pickByTree(surfaces, 0, point, normal, 0.5F, u);
                if (picked.place == kernel::geometry::kNoPrimitive) {
                    ++none;
                    continue;
                }
                ++counts[{ picked.place, picked.primitive }];
                const float reported = lights.chanceByTree(surfaces, 0, picked.place, picked.primitive, point, normal, 0.5F);
                furthest = std::fmax(furthest, std::fabs(picked.chance - reported) / reported);
            }
            BOOST_TEST(furthest < 1.0e-5F);
            double sum = 0.0;
            for (std::uint32_t place = 0; place < lights.lights.size; ++place) {
                const kernel::light::Light & light = lights.lights[place];
                const bool mesh = light.shape == kernel::light::Shape::Mesh;
                for (std::uint32_t triangle = 0; triangle < (mesh ? light.count : 1U); ++triangle) {
                    const std::uint32_t primitive = mesh ? triangle : kernel::geometry::kNoPrimitive;
                    const double chance = lights.chanceByTree(surfaces, 0, place, primitive, point, normal, 0.5F);
                    sum += chance;
                    const auto drawn = counts.find({ place, primitive });
                    const double times = drawn == counts.end() ? 0.0 : drawn->second;
                    BOOST_TEST(std::fabs(times - (chance * kDraws)) <= 2.0, "light " << place << " triangle " << triangle);
                }
            }
            BOOST_TEST(sum > 0.5);
            BOOST_TEST(std::fabs(none - ((1.0 - sum) * kDraws)) <= 2.0);
        }
    }
}

// the light tree is the same node for node, whatever threads the device runs
BOOST_AUTO_TEST_CASE(moya_lights_the_light_tree_is_the_same_on_any_count_of_threads_test) {
    scene::Scene scene(scene::kinds());
    everyKind(scene, "mis", "tree", 1, true);
    scene.commit();
    scene.apply();
    const Built one(scene, 1);
    const Built many(scene, 8);
    const kernel::light::Lights left = one.lights.view();
    const kernel::light::Lights right = many.lights.view();
    BOOST_TEST(left.rootOf(0) == right.rootOf(0));
    BOOST_TEST(left.nodes.size > 1U);
    BOOST_TEST_REQUIRE(left.nodes.size == right.nodes.size);
    BOOST_TEST(std::memcmp(left.nodes.data, right.nodes.data, left.nodes.size * sizeof(kernel::light::TreeNode)) == 0);
    BOOST_TEST_REQUIRE(left.leaves.size == right.leaves.size);
    BOOST_TEST(std::equal(left.leaves.data, left.leaves.data + left.leaves.size, right.leaves.data));
    BOOST_TEST_REQUIRE(left.triangleLeaves.size == right.triangleLeaves.size);
    BOOST_TEST(std::equal(left.triangleLeaves.data, left.triangleLeaves.data + left.triangleLeaves.size, right.triangleLeaves.data));
}

// a plane under a grid of small sphere lights, one of which hangs just above it, has a lower
// error through the light tree than by drawing in proportion to power at the same samples. By
// power the near light takes one draw in sixty-five, wherever the shading point lies
BOOST_AUTO_TEST_CASE(moya_lights_the_light_tree_lowers_the_error_under_many_small_lights_test) {
    const auto error = [](const char * picking) {
        scene::Scene scene(scene::kinds());
        const scene::Handle options = litPlane(scene, "mis", 16, 0.5F);
        scene.set(options, "lightPicking", scene::Value::string(picking));
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                const scene::Handle far = scene.create("sphereLight", "far" + std::to_string((y * 8) + x));
                scene.set(far, "radius", scene::Value::number(0.02F));
                scene.set(far, "transform", scene::Value::matrix(moved(static_cast<float>(x) - 3.5F, static_cast<float>(y) - 3.5F, -3.0F)));
            }
        }
        const scene::Handle near = scene.create("sphereLight", "near");
        scene.set(near, "radius", scene::Value::number(0.02F));
        scene.set(near, "transform", scene::Value::matrix(moved(0.1F, 0.1F, -0.2F)));
        const film::Snapshot snapshot = rendered(scene);
        double sum = 0.0;
        for (std::uint32_t y = 0; y < kSide; ++y) {
            for (std::uint32_t x = 0; x < kSide; ++x) {
                sum += film::luminance(snapshot, x, y).error;
            }
        }
        return sum / (kSide * kSide);
    };
    BOOST_TEST(error("tree") < 0.5 * error("power"));
}

/**
 * The sum of the luminance of the pixels whose centres lie within a radius of a point, as a camera
 * views a square of a half width about the origin. The picture's first row is the square's +y.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the half width, then the radius
float sumInside(const film::Snapshot & snapshot, float half, const glm::vec2 & centre, float radius) {
    float sum = 0.0F;
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            const float step = 2.0F * half / static_cast<float>(kSide);
            const glm::vec2 at { -half + ((static_cast<float>(x) + 0.5F) * step), half - ((static_cast<float>(y) + 0.5F) * step) };
            if (glm::distance(at, centre) < radius) {
                sum += film::luminance(snapshot, x, y).value;
            }
        }
    }
    return sum;
}

// a light linked to one of two spheres lights that sphere alone, by each strategy and each way of
// picking: a disk light, and a dome light beside the tree. The other sphere is black, since a
// diffuse limit of one leaves it no light by way of the first. Each sphere is measured well inside
// its edge, where the pixel filter reaches no light behind it
BOOST_AUTO_TEST_CASE(moya_lights_a_linked_light_lights_only_the_instances_it_names_test) {
    for (const char * kind : { "diskLight", "domeLight" }) {
        for (const char * picking : { "tree", "power" }) {
            for (const char * strategy : { "light", "bsdf", "mis" }) {
                BOOST_TEST_CONTEXT(kind << " " << picking << " " << strategy) {
                    scene::Scene scene(scene::kinds());
                    const scene::Handle options = viewed(scene, 16, 5.0F, glm::vec2 { 0.0F }, 1.5F);
                    scene.set(options, "lightSampling", scene::Value::string(strategy));
                    scene.set(options, "lightPicking", scene::Value::string(picking));
                    scene.set(options, "diffuseDepth", scene::Value::integer(1));
                    const scene::Handle ball = scene.create("sphere", "ball");
                    scene.set(ball, "radius", scene::Value::number(0.5F));
                    const scene::Handle left = instanced(scene, "left", ball, {}, moved(-0.8F, 0.0F, 0.0F));
                    instanced(scene, "right", ball, {}, moved(0.8F, 0.0F, 0.0F));
                    const scene::Handle light = std::string(kind) == "diskLight" ? diskAbove(scene, 1.0F, 2.0F) : sky(scene, "");
                    scene.set(light, "includeRoot", scene::Value::integer(0));
                    scene.set(light, "instances", scene::Value::handles({ left }));
                    const film::Snapshot snapshot = rendered(scene);
                    BOOST_TEST(sumInside(snapshot, 1.5F, glm::vec2 { -0.8F, 0.0F }, 0.3F) > 0.0F);
                    BOOST_TEST(sumInside(snapshot, 1.5F, glm::vec2 { 0.8F, 0.0F }, 0.3F) == 0.0F);
                }
            }
        }
    }
}

// a light names an instance in a group, and lights what that instance places wherever the group
// is placed. A second placing of the group's other member is left dark
BOOST_AUTO_TEST_CASE(moya_lights_a_linked_light_lights_an_instance_it_names_within_a_group_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle options = viewed(scene, 16, 5.0F, glm::vec2 { 0.0F }, 1.5F);
    scene.set(options, "diffuseDepth", scene::Value::integer(1));
    const scene::Handle ball = scene.create("sphere", "ball");
    scene.set(ball, "radius", scene::Value::number(0.4F));
    const scene::Handle group = scene.create("group", "pair");
    const scene::Handle upper = instanced(scene, "upper", ball, group, moved(0.0F, -0.6F, 0.0F));
    instanced(scene, "lower", ball, group, moved(0.0F, 0.6F, 0.0F));
    instanced(scene, "west", group, {}, moved(-0.8F, 0.0F, 0.0F));
    instanced(scene, "east", group, {}, moved(0.8F, 0.0F, 0.0F));
    const scene::Handle light = diskAbove(scene, 1.0F, 2.0F);
    scene.set(light, "includeRoot", scene::Value::integer(0));
    scene.set(light, "instances", scene::Value::handles({ upper }));
    const film::Snapshot snapshot = rendered(scene);
    for (const float x : { -0.8F, 0.8F }) {
        BOOST_TEST(sumInside(snapshot, 1.5F, glm::vec2 { x, -0.6F }, 0.2F) > 0.0F);
        BOOST_TEST(sumInside(snapshot, 1.5F, glm::vec2 { x, 0.6F }, 0.2F) == 0.0F);
    }
}

/**
 * A white plane under two disk lights, found by a strategy, with a black disk wider than the plane
 * between them that camera rays see through. The first light's shadow link names the cover while
 * castsFirst holds, and nothing otherwise. The second's names every instance. Without the cover,
 * the first light is alone.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - covered, then whether it casts
film::Snapshot shadowLinked(const char * strategy, bool covered, bool castsFirst) {
    scene::Scene scene(scene::kinds());
    const scene::Handle options = viewed(scene, 64, 5.0F, glm::vec2 { 0.0F }, 0.25F);
    scene.set(options, "lightSampling", scene::Value::string(strategy));
    plane(scene);
    const scene::Handle first = diskAbove(scene, 0.5F, 2.0F);
    if (!covered) {
        return rendered(scene);
    }
    const scene::Handle cover = scene.create("disk", "cover");
    scene.set(cover, "radius", scene::Value::number(200.0F));
    black(scene, cover);
    const scene::Handle placed = instanced(scene, "cover", cover, {}, moved(0.0F, 0.0F, -1.0F));
    scene.set(placed, "camera", scene::Value::integer(0));
    scene.set(first, "shadowIncludeRoot", scene::Value::integer(0));
    scene.set(first, "shadowInstances", castsFirst ? scene::Value::handles({ placed }) : scene::Value::handles({}));
    const scene::Handle second = scene.create("diskLight", "second");
    scene.set(second, "radius", scene::Value::number(0.5F));
    scene.set(second, "transform", scene::Value::matrix(flipped(moved(0.3F, 0.0F, -2.0F))));
    return rendered(scene);
}

// a light's shadow link that leaves the cover out lights the plane through it, as though the
// cover were not there, by each strategy. A second light is still shadowed by the cover, and the
// first is too once its shadow link names the cover
BOOST_AUTO_TEST_CASE(moya_lights_a_shadow_link_casts_shadows_only_from_the_instances_it_names_test) {
    for (const char * strategy : { "light", "bsdf", "mis" }) {
        BOOST_TEST_CONTEXT(strategy) {
            const film::Snapshot open = shadowLinked(strategy, false, false);
            const film::Snapshot through = shadowLinked(strategy, true, false);
            BOOST_TEST(film::mean(open, { 0, 0 }, { kSide, kSide }).value > 0.0F);
            BOOST_TEST(film::sigmas(film::mean(through, { 0, 0 }, { kSide, kSide }), film::mean(open, { 0, 0 }, { kSide, kSide })) < 3.0F);
            BOOST_TEST(film::furthest(through, open).sigmas < 5.0F);
            BOOST_TEST(film::mean(shadowLinked(strategy, true, true), { 0, 0 }, { kSide, kSide }).value == 0.0F);
        }
    }
}

/** The numbers of an IES file after its TILT line, of type C photometry and a multiplier of 1. **/
std::string iesNumbers(const std::vector<float> & vertical, const std::vector<float> & horizontal, const std::vector<float> & candela) {
    std::string out = "1 -1 1 " + std::to_string(vertical.size()) + " " + std::to_string(horizontal.size()) + " 1 1 0 0 0\n1 1 0\n";
    for (const std::vector<float> * values : { &vertical, &horizontal, &candela }) {
        for (const float value : *values) {
            out += std::to_string(value) + " ";
        }
        out += "\n";
    }
    return out;
}

/** An IES file of a name in the temporary directory, holding a text. Gives its path. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the name, then the text
std::string iesFile(const std::string & name, const std::string & text) {
    const std::string path = (boost::filesystem::temp_directory_path() / ("v3d_lights_" + name + ".ies")).string();
    std::ofstream(path, std::ios::binary) << text;
    return path;
}

/** An IES file of LM-63-2002 with no tilt, of type C photometry. Gives its path. **/
std::string iesFile(const std::string & name, const std::vector<float> & vertical, const std::vector<float> & horizontal, const std::vector<float> & candela) {
    return iesFile(name, "IESNA:LM-63-2002\n[TEST] moya\nTILT=NONE\n" + iesNumbers(vertical, horizontal, candela));
}

/**
 * A white plane under a sphere light of a radius at a height, viewed across a half width. The
 * light's -z faces the plane while facing holds, and an IES file shapes it when one is named.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the radius, the height, then the width
film::Snapshot underProfile(const std::string & path, float radius, float height, float half, bool facing, bool normalise = false) {
    scene::Scene scene(scene::kinds());
    litPlane(scene, "mis", 64, half);
    const scene::Handle sphere = bulb(scene, radius, height);
    if (facing) {
        scene.set(sphere, "transform", scene::Value::matrix(flipped(moved(0.0F, 0.0F, -height))));
    }
    scene.set(sphere, "iesFile", scene::Value::string(path));
    scene.set(sphere, "iesNormalize", scene::Value::integer(normalise ? 1 : 0));
    return rendered(scene);
}

// an IES file of one candela in every direction leaves a light as it was, and one of 250 candela
// does too once normalised. A point and a sphere light are shaped alike
BOOST_AUTO_TEST_CASE(moya_lights_a_uniform_ies_profile_leaves_a_light_as_it_was_test) {
    const std::string one = iesFile("uniform", { 0.0F, 180.0F }, { 0.0F }, { 1.0F, 1.0F });
    const std::string many = iesFile("bright", { 0.0F, 90.0F, 180.0F }, { 0.0F, 90.0F }, { 250.0F, 250.0F, 250.0F, 250.0F, 250.0F, 250.0F });
    for (const float radius : { 0.0F, 0.2F }) {
        BOOST_TEST_CONTEXT("radius " << radius) {
            const film::Snapshot plain = underProfile("", radius, 1.0F, 0.5F, false);
            for (const film::Snapshot & shaped : { underProfile(one, radius, 1.0F, 0.5F, false), underProfile(many, radius, 1.0F, 0.5F, false, true) }) {
                BOOST_TEST(film::sigmas(film::mean(shaped, { 0, 0 }, { kSide, kSide }), film::mean(plain, { 0, 0 }, { kSide, kSide })) < 3.0F);
                BOOST_TEST(film::furthest(shaped, plain).sigmas < 5.0F);
            }
        }
    }
}

// a profile dark on its nadir's side lights nothing that the light's -z faces, and lights the
// plane once the light turns its -z away. A cylinder light is shaped from its own -z, as a
// sphere light is, though its shape lies along its prototype's z
BOOST_AUTO_TEST_CASE(moya_lights_an_ies_profile_is_measured_from_the_lights_minus_z_test) {
    const std::string upward = iesFile("upward", { 0.0F, 89.0F, 91.0F, 180.0F }, { 0.0F }, { 0.0F, 0.0F, 1.0F, 1.0F });
    for (const char * kind : { "sphereLight", "cylinderLight" }) {
        for (const bool facing : { true, false }) {
            BOOST_TEST_CONTEXT(kind << (facing ? " facing" : " turned away")) {
                scene::Scene scene(scene::kinds());
                litPlane(scene, "mis", 16, 0.5F);
                const scene::Handle light = scene.create(kind, "shaped");
                scene.set(light, "radius", scene::Value::number(0.1F));
                const glm::mat4 place = moved(0.0F, 0.0F, -1.0F);
                scene.set(light, "transform", scene::Value::matrix(facing ? flipped(place) : place));
                scene.set(light, "iesFile", scene::Value::string(upward));
                const float lit = film::mean(rendered(scene), { 0, 0 }, { kSide, kSide }).value;
                BOOST_TEST((facing ? lit == 0.0F : lit > 0.0F));
            }
        }
    }
}

// a profile's horizontal angle runs about the light's -z from its +x towards its +y. A profile lit
// only between 0 and 44 degrees lights a wedge of the plane beside the point's +x, and none beside
// its +y or past its -x. The point is turned half about x to face the plane, so its +y is the
// plane's -y, which the picture's lower half shows
BOOST_AUTO_TEST_CASE(moya_lights_an_ies_profiles_horizontal_angle_runs_from_the_lights_plus_x_test) {
    const std::string wedge = iesFile("wedge", { 0.0F, 180.0F }, { 0.0F, 44.0F, 46.0F, 360.0F }, { 1.0F, 1.0F, 1.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F });
    const film::Snapshot snapshot = underProfile(wedge, 0.0F, 1.0F, 1.5F, true);
    // the plane's +x beside its -y, then its -y beside its +x, then its -x
    BOOST_TEST(film::mean(snapshot, { 21, 17 }, { kSide, 20 }).value > 0.0F);
    BOOST_TEST(film::mean(snapshot, { 17, 22 }, { 19, kSide - 1 }).value == 0.0F);
    BOOST_TEST(film::mean(snapshot, { 0, 0 }, { 14, kSide }).value == 0.0F);
}

// a positive angle scale narrows a profile about its nadir. A profile lit within 30 degrees of its
// nadir lights a ring of the plane between 22 and 26 degrees off a point's axis, and none of it
// once its angle scale halves its spread. What lies within 6 degrees is lit either way. The ring
// lies more than a pixel past the narrowed edge, so the pixel filter carries no light into it
BOOST_AUTO_TEST_CASE(moya_lights_an_ies_angle_scale_narrows_a_profile_test) {
    const std::string spot = iesFile("spot", { 0.0F, 29.0F, 31.0F, 180.0F }, { 0.0F }, { 1.0F, 1.0F, 0.0F, 0.0F });
    for (const float scale : { 0.0F, 0.5F }) {
        BOOST_TEST_CONTEXT("angle scale " << scale) {
            scene::Scene scene(scene::kinds());
            litPlane(scene, "mis", 16, 1.5F);
            const scene::Handle point = bulb(scene, 0.0F, 1.0F);
            scene.set(point, "transform", scene::Value::matrix(flipped(moved(0.0F, 0.0F, -1.0F))));
            scene.set(point, "iesFile", scene::Value::string(spot));
            scene.set(point, "iesAngleScale", scene::Value::number(scale));
            const film::Snapshot snapshot = rendered(scene);
            const float ring = sumInside(snapshot, 1.5F, glm::vec2 { 0.0F }, 0.48F) - sumInside(snapshot, 1.5F, glm::vec2 { 0.0F }, 0.4F);
            BOOST_TEST(sumInside(snapshot, 1.5F, glm::vec2 { 0.0F }, 0.12F) > 0.0F);
            BOOST_TEST((scale > 0.0F ? ring == 0.0F : ring > 0.0F));
        }
    }
}

// LM-63's numbers may be parted by commas and line breaks alike, and the tilt an INCLUDE gives is
// skipped. Each of these files is refused with why: one with no TILT line, one whose TILT names
// another file, and one of type B photometry. A file is refused for angles that do not rise, a span
// type C does not allow, a candela below zero, or for being cut short
BOOST_AUTO_TEST_CASE(moya_lights_an_ies_file_is_read_as_lm_63_says_test) {
    const std::string numbers = iesNumbers({ 0.0F, 90.0F }, { 0.0F, 180.0F }, { 1.0F, 2.0F, 3.0F, 4.0F });
    light::Photometry plain;
    std::string why;
    BOOST_TEST_REQUIRE(light::parseIes("IESNA91\nTILT=NONE\n" + numbers, plain, why));
    BOOST_TEST((plain.candela == std::vector<float> { 1.0F, 2.0F, 3.0F, 4.0F }));
    BOOST_TEST(std::fabs(plain.horizontal.back() - kPi) < 1.0e-6F);
    std::string commas = numbers;
    std::replace(commas.begin(), commas.end(), ' ', ',');
    light::Photometry tilted;
    BOOST_TEST_REQUIRE(light::parseIes("[TEST] tilted\nTILT=INCLUDE\n1\n2\n0 90\n1 0.5\n" + commas, tilted, why));
    BOOST_TEST((tilted.candela == plain.candela));
    BOOST_TEST((tilted.vertical == plain.vertical));
    const std::vector<std::pair<std::string, std::string>> refused {
        { "1 -1 1 2 2 1 1 0 0 0\n", "no TILT line" },
        { "TILT=lamp.tlt\n" + numbers, "names the file lamp.tlt" },
        { "TILT=NONE\n1 -1 1 2 2 2 1 0 0 0\n1 1 0\n0 90\n0 180\n1 2 3 4\n", "not of type C" },
        { "TILT=NONE\n" + iesNumbers({ 90.0F, 0.0F }, { 0.0F }, { 1.0F, 1.0F }), "vertical angles do not rise" },
        { "TILT=NONE\n" + iesNumbers({ 0.0F, 90.0F }, { 0.0F, 120.0F }, { 1.0F, 1.0F, 1.0F, 1.0F }), "horizontal angles" },
        { "TILT=NONE\n" + iesNumbers({ 0.0F, 90.0F }, { 0.0F }, { 1.0F, -1.0F }), "below zero" },
        { "TILT=NONE\n1 -1 1 2 1 1 1 0 0 0\n1 1 0\n0 90\n0\n1\n", "cut short" },
    };
    for (const auto & [text, reason] : refused) {
        BOOST_TEST_CONTEXT(reason) {
            light::Photometry read;
            std::string said;
            BOOST_TEST(!light::parseIes(text, read, said));
            BOOST_TEST(said.find(reason) != std::string::npos, said);
        }
    }
}

// a light that names a malformed IES file is reported, and lit as though it named none
BOOST_AUTO_TEST_CASE(moya_lights_a_malformed_ies_file_is_reported_and_the_light_unshaped_test) {
    const std::string broken = iesFile("broken", "IESNA:LM-63-2002\nTILT=NONE\n1 -1 1 two\n");
    scene::Scene scene(scene::kinds());
    litPlane(scene, "mis", 64, 0.5F);
    scene.set(bulb(scene, 0.2F, 1.0F), "iesFile", scene::Value::string(broken));
    scene.commit();
    scene.apply();
    const auto logger = boost::make_shared<v3d::log::Logger>();
    const handling::Heard heard(logger->get());
    CpuDevice device;
    const film::Snapshot shaped = integrate::render(device, scene, logger, colour::Config());
    BOOST_TEST(heard.said("cannot be read"));
    const film::Snapshot plain = underProfile("", 0.2F, 1.0F, 0.5F, false);
    BOOST_TEST(film::furthest(shaped, plain).sigmas < 5.0F);
}

};  // namespace

};  // namespace v3d::moya::tracing

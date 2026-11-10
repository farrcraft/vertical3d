/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/Assembler.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Compare.h>
#include <moya/libmoya/film/Estimate.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/geometry/Group.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/World.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/Emission.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/light/Planckian.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

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

/** A half turn about x, which turns -z onto +z. **/
glm::mat4 flipped(const glm::mat4 & place) {
    return glm::rotate(place, glm::radians(180.0F), glm::vec3 { 1.0F, 0.0F, 0.0F });
}

/**
 * A scene of an orthographic camera at a distance back along -z, which looks along +z. It sees
 * the square of a half width about a centre in x and y, at kSide pixels square, with samples per
 * pixel and a black environment.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the samples, then the view
void viewed(scene::Scene & scene, std::uint32_t samples, float distance, const glm::vec2 & centre, float half) {
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(options, "resolution", scene::Value::integers({ kSide, kSide }));
    scene.set(options, "samples", scene::Value::integer(static_cast<std::int32_t>(samples)));
    scene.set(camera, "screen", scene::Value::floats({ centre.x - half, centre.x + half, centre.y - half, centre.y + half }));
    scene.set(camera, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -distance)));
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

/** A light camera rays see, at the origin, in a scene viewed from five units back. **/
scene::Handle seen(scene::Scene & scene, const std::string & kind, const glm::mat4 & place) {
    viewed(scene, 16, 5.0F, glm::vec2 { 0.0F }, 1.0F);
    const scene::Handle made = scene.create(kind, "seen");
    scene.set(made, "camera", scene::Value::integer(1));
    scene.set(made, "normalize", scene::Value::integer(1));
    scene.set(made, "transform", scene::Value::matrix(place));
    return made;
}

// a white plane under a disk light reaches the closed form of the disk's irradiance, by paths
// that sample the plane's BSDF and meet the light. The camera sees through the light
BOOST_AUTO_TEST_CASE(moya_lights_a_disk_light_gives_its_closed_form_by_bsdf_sampling_test) {
    const glm::vec2 centre { 0.0F };
    scene::Scene scene(scene::kinds());
    viewed(scene, 256, 5.0F, centre, 0.25F);
    plane(scene);
    diskAbove(scene, 2.0F, 1.0F);
    const film::Snapshot snapshot = rendered(scene);
    const film::Furthest furthest = film::furthest(snapshot, [&](const glm::uvec2 & pixel) {
        const glm::vec2 at = centre - 0.25F + (glm::vec2 { static_cast<float>(pixel.x), static_cast<float>(pixel.y) } + 0.5F) * (0.5F / static_cast<float>(kSide));
        return underDisk(glm::length(at), 2.0F, 1.0F);
    });
    BOOST_TEST(furthest.sigmas < 5.0F);
    const float expected = meanOver(centre, 0.25F, &diskAt, 2.0F, 1.0F);
    BOOST_TEST(film::sigmas(film::mean(snapshot, { 0, 0 }, { kSide, kSide }), expected) < 3.0F);
    BOOST_TEST(film::sigmas(film::mean(snapshot, { 0, 0 }, { kSide, kSide }), expected * 1.05F) > 10.0F);
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
    BOOST_TEST(film::sigmas(small, large) < 3.0F);
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

// each light with a shape is an instance in the world, after every other, with the light's index
// in the scene. It is visible to indirect rays, to camera rays when set, and never to shadow rays
BOOST_AUTO_TEST_CASE(moya_lights_each_shaped_light_is_an_instance_in_the_world_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle sphere = scene.create("sphereLight", "a");
    const scene::Handle cylinder = scene.create("cylinderLight", "b");
    scene.set(cylinder, "camera", scene::Value::integer(1));
    scene.create("distantLight", "sun");
    const scene::Handle ball = scene.create("sphere", "ball");
    instanced(scene, "ball", ball, {}, glm::mat4(1.0F));
    scene.commit();
    scene.apply();
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
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

};  // namespace

};  // namespace v3d::moya::tracing

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/device/cpu/Isa.h>
#include <moya/libmoya/device/CpuLibrary.h>
#include <moya/libmoya/film/Film.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Polynomial.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Shapes.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/view/Hits.h>
#include <moya/libmoya/view/Tracing.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

namespace {

namespace cpu = v3d::moya::device::cpu;
namespace geometry = v3d::moya::geometry;
namespace kernel = v3d::moya::kernel;
namespace scene = v3d::moya::scene;
namespace view = v3d::moya::view;
typedef v3d::moya::device::CpuDevice CpuDevice;
typedef kernel::geometry::Shape Shape;

/** A random number in [0, 1) from a seed it moves on, the same on every compiler. **/
float next(std::uint32_t & seed) {
    seed = (seed * 1664525U) + 1013904223U;
    return static_cast<float>(seed >> 8U) / static_cast<float>(1U << 24U);
}

/** The instruction sets this processor runs, oldest first. **/
std::vector<cpu::Isa> runnable() {
    std::vector<cpu::Isa> out;
    for (const cpu::Isa isa : { cpu::Isa::Sse42, cpu::Isa::Avx2, cpu::Isa::Avx512 }) {
        if (isa <= v3d::moya::device::newestIsa()) {
            out.push_back(isa);
        }
    }
    return out;
}

/** The name of a traversal at an instruction set, for a failure's message. **/
std::string tracerName(view::Tracer tracer, cpu::Isa isa) {
    return tracer == view::Tracer::BruteForce ? std::string("brute force") : v3d::moya::device::isaName(isa);
}

/** Runs a check on the brute-force traversal, and on the hierarchy at each instruction set. **/
template <typename F>
void eachTraversal(F && check) {
    check(view::Tracer::BruteForce, cpu::Isa::Sse42);
    for (const cpu::Isa isa : runnable()) {
        check(view::Tracer::Hierarchy, isa);
    }
}

/** An instance of a prototype in the world, with its transformation at each of its times. **/
scene::Handle instanced(scene::Scene & scene, const std::string & name, const scene::Handle & prototype, const std::vector<float> & times,
    const std::vector<glm::mat4> & transforms) {
    const scene::Handle instance = scene.create("instance", name);
    scene.set(instance, "prototype", scene::Value::handle(prototype));
    if (times.empty()) {
        scene.set(instance, "transform", scene::Value::matrix(transforms.front()));
    } else {
        std::vector<scene::Value> values;
        values.reserve(transforms.size());
        for (const glm::mat4 & transform : transforms) {
            values.push_back(scene::Value::matrix(transform));
        }
        scene.set(instance, "transform", times, values);
    }
    return instance;
}

/** The scene's camera, whose shutter opens at 0 and closes at 1. **/
void shutter(scene::Scene & scene) {
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(camera, "shutter", scene::Value::floats({ 0.0F, 1.0F }));
}

/** Each of the seven quadrics, cut and swept short of a whole turn, so every limit is met. **/
std::vector<kernel::geometry::Quadric> quadrics() {
    const float sweep = glm::radians(270.0F);
    const kernel::geometry::Quadric sphere { .shape = Shape::Sphere, .radius = 1.5F, .zmin = -1.0F, .zmax = 1.2F, .thetamax = sweep };
    const kernel::geometry::Quadric cone { .shape = Shape::Cone, .radius = 1.0F, .height = 2.0F, .thetamax = sweep };
    const kernel::geometry::Quadric cylinder { .shape = Shape::Cylinder, .radius = 1.0F, .zmin = -1.0F, .zmax = 1.5F, .thetamax = sweep };
    const kernel::geometry::Quadric hyperboloid { .shape = Shape::Hyperboloid, .point1 = { 1.0F, -1.0F, -1.0F }, .point2 = { 1.0F, 1.0F, 1.0F },
        .thetamax = sweep };
    const kernel::geometry::Quadric paraboloid { .shape = Shape::Paraboloid, .radius = 1.0F, .zmin = 0.2F, .zmax = 1.5F, .thetamax = sweep };
    const kernel::geometry::Quadric disk { .shape = Shape::Disk, .radius = 1.2F, .height = 0.3F, .thetamax = sweep };
    const kernel::geometry::Quadric torus { .shape = Shape::Torus, .radius = 1.0F, .minorRadius = 0.3F, .phimin = glm::radians(-90.0F),
        .phimax = glm::radians(180.0F), .thetamax = sweep };
    return { sphere, cone, cylinder, hyperboloid, paraboloid, disk, torus };
}

/** How far a point lies from a quadric's surface of revolution, ignoring its limits. **/
float offSurface(const kernel::geometry::Quadric & quadric, const glm::vec3 & point) {
    const float reach = std::sqrt((point.x * point.x) + (point.y * point.y));
    switch (quadric.shape) {
        case Shape::Sphere:
            return glm::length(point) - quadric.radius;
        case Shape::Cylinder:
            return reach - quadric.radius;
        case Shape::Cone:
            return reach - (quadric.radius * (1.0F - (point.z / quadric.height)));
        case Shape::Paraboloid:
            return reach - std::sqrt(std::fmax(0.0F, (quadric.radius * quadric.radius) * point.z / quadric.zmax));
        case Shape::Hyperboloid: {
            const float v = (point.z - quadric.point1.z) / (quadric.point2.z - quadric.point1.z);
            const glm::vec3 line = quadric.point1 + ((quadric.point2 - quadric.point1) * v);
            return reach - std::sqrt((line.x * line.x) + (line.y * line.y));
        }
        case Shape::Disk:
            return point.z - quadric.height;
        case Shape::Torus:
            return std::sqrt(((reach - quadric.radius) * (reach - quadric.radius)) + (point.z * point.z)) - quadric.minorRadius;
    }
    return 0.0F;
}

// the four roots of (x - 1)(x - 2)(x - 3)(x - 4) are found in order, and only those in the
// interval asked for
BOOST_AUTO_TEST_CASE(moya_shapes_a_quartics_roots_are_found_in_order_test) {
    const float quartic[5] = { 24.0F, -50.0F, 35.0F, -10.0F, 1.0F };
    float roots[4] {};
    BOOST_TEST(kernel::geometry::rootsIn(quartic, 4, 0.0F, 5.0F, roots) == 4U);
    for (std::uint32_t root = 0; root < 4; ++root) {
        BOOST_TEST(std::fabs(roots[root] - static_cast<float>(root + 1)) < 1.0e-5F, "root " << root);
    }
    BOOST_TEST(kernel::geometry::rootsIn(quartic, 4, 1.5F, 3.5F, roots) == 2U);
    BOOST_TEST(std::fabs(roots[0] - 2.0F) < 1.0e-5F);
    // x^2 + 1 has no real root
    const float none[3] = { 1.0F, 0.0F, 1.0F };
    BOOST_TEST(kernel::geometry::rootsIn(none, 2, -10.0F, 10.0F, roots) == 0U);
}

// rays from all around each quadric hit it only on its surface of revolution, within its sweep and
// its limits, and a good share of them hit
BOOST_AUTO_TEST_CASE(moya_shapes_each_quadrics_hits_lie_on_its_surface_and_within_its_sweep_test) {
    std::uint32_t seed = 13;
    for (const kernel::geometry::Quadric & quadric : quadrics()) {
        std::size_t hits = 0;
        std::size_t off = 0;
        std::size_t outside = 0;
        for (int i = 0; i < 4000; ++i) {
            kernel::geometry::Ray ray;
            ray.origin = glm::normalize(glm::vec3 { next(seed), next(seed), next(seed) } - 0.5F) * 6.0F;
            const glm::vec3 target = (glm::vec3 { next(seed), next(seed), next(seed) } - 0.5F) * 3.0F;
            ray.direction = target - ray.origin;
            kernel::geometry::ShapeHit hit;
            if (!kernel::geometry::intersectQuadric(quadric, ray, 0.0F, 10.0F, hit)) {
                continue;
            }
            ++hits;
            const glm::vec3 point = ray.at(hit.distance);
            off += std::fabs(offSurface(quadric, point)) > 1.0e-3F ? 1 : 0;
            glm::vec2 uv { 0.0F };
            outside += kernel::geometry::quadricPosition(quadric, point, uv) && uv.x <= 1.0F + 1.0e-5F ? 0 : 1;
        }
        const auto shape = static_cast<int>(quadric.shape);
        BOOST_TEST(hits > 400U, "shape " << shape);
        BOOST_TEST(off == 0U, "shape " << shape);
        BOOST_TEST(outside == 0U, "shape " << shape);
    }
}

// a ray down a torus's axis passes through its hole, and one down through its tube meets the
// tube's top
BOOST_AUTO_TEST_CASE(moya_shapes_a_ray_along_a_toruss_axis_misses_it_test) {
    const kernel::geometry::Quadric torus { .shape = Shape::Torus, .radius = 1.0F, .minorRadius = 0.25F };
    kernel::geometry::Ray ray;
    ray.origin = glm::vec3 { 0.0F, 0.0F, 5.0F };
    ray.direction = glm::vec3 { 0.0F, 0.0F, -1.0F };
    kernel::geometry::ShapeHit hit;
    BOOST_TEST(!kernel::geometry::intersectQuadric(torus, ray, 0.0F, 100.0F, hit));
    ray.origin.x = 1.0F;
    BOOST_TEST(kernel::geometry::intersectQuadric(torus, ray, 0.0F, 100.0F, hit));
    BOOST_TEST(std::fabs(hit.distance - 4.75F) < 1.0e-4F);
}

// a flat point of width two is met from every direction within its radius of its centre, and
// missed just beyond it. It is met in the plane through its centre that faces the ray, and a
// round point at its surface
BOOST_AUTO_TEST_CASE(moya_shapes_a_flat_point_is_hit_from_any_direction_at_its_width_test) {
    for (const char * profile : { "flat", "round" }) {
        scene::Scene scene(scene::kinds());
        const scene::Handle cloud = scene.create("points", "cloud");
        const glm::vec3 centre { 1.0F, 2.0F, 3.0F };
        scene.set(cloud, "P", scene::Value::points({ centre }));
        scene.set(cloud, "constantWidth", scene::Value::number(2.0F));
        scene.set(cloud, "profile", scene::Value::string(profile));
        instanced(scene, "cloud", cloud, {}, { glm::mat4(1.0F) });
        scene.commit();
        scene.apply();
        const geometry::Prototypes prototypes = geometry::gather(scene, {});

        std::uint32_t seed = 3;
        std::vector<kernel::geometry::Ray> rays;
        for (int i = 0; i < 200; ++i) {
            const glm::vec3 from = glm::normalize(glm::vec3 { next(seed), next(seed), next(seed) } - 0.5F);
            // an offset across the ray's path, inside the radius for even rays and outside for odd
            const glm::vec3 across = glm::normalize(glm::cross(from, glm::vec3 { 0.3F, 0.5F, 0.8F })) * (i % 2 == 0 ? 0.99F : 1.01F);
            kernel::geometry::Ray ray;
            ray.origin = centre + (from * 10.0F) + across;
            ray.direction = -from;
            rays.push_back(ray);
        }
        eachTraversal([&](view::Tracer tracer, cpu::Isa isa) {
            CpuDevice on(0, isa);
            const view::Traced traced = view::trace(on, prototypes, rays, tracer);
            // a ray passes 0.99 from the centre, so it meets a sphere that far short of the plane
            const float distance = std::string(profile) == "flat" ? 10.0F : 10.0F - std::sqrt(1.0F - (0.99F * 0.99F));
            std::size_t wrong = 0;
            for (std::size_t i = 0; i < rays.size(); ++i) {
                wrong += traced.nearest[i].found() == (i % 2 == 0) ? 0 : 1;
                wrong += i % 2 == 0 && std::fabs(traced.nearest[i].distance - distance) > 1.0e-3F ? 1 : 0;
            }
            BOOST_TEST(wrong == 0U, profile << ", " << tracerName(tracer, isa));
        });
    }
}

/**
 * Whether two traversals give one answer: both miss, or both meet the same primitive through the
 * same chain at distances within a thousandth.
 **/
bool agrees(const kernel::geometry::Hit & want, const kernel::geometry::Hit & got) {
    if (want.found() != got.found()) {
        return false;
    }
    if (!want.found()) {
        return true;
    }
    const bool chain = want.depth == got.depth && std::equal(want.instances, want.instances + want.depth, got.instances);
    return chain && want.primitive == got.primitive && std::fabs(want.distance - got.distance) <= 1.0e-3F * std::max(1.0F, want.distance);
}

/** A quadric of the scene with a dimension that moves over the shutter, and RI's defaults. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the kind, then the dimension that moves
scene::Handle movingQuadric(scene::Scene & scene, const std::string & kind, const std::string & dimension, float from, float to) {
    const scene::Handle out = scene.create(kind, kind);
    scene.set(out, dimension, { 0.0F, 1.0F }, { scene::Value::number(from), scene::Value::number(to) });
    scene.set(out, "thetamax", scene::Value::number(300.0F));
    return out;
}

// a scene holds every quadric and points round and flat, each moving, stretched, or both. The
// hierarchy hits it as the brute-force traversal hits the scene placed at each of several times,
// at every instruction set
BOOST_AUTO_TEST_CASE(moya_shapes_the_hierarchy_finds_the_oracles_nearest_hit_on_points_and_quadrics_test) {
    std::uint32_t seed = 41;
    scene::Scene scene(scene::kinds());
    shutter(scene);
    std::vector<scene::Handle> prototypes {
        movingQuadric(scene, "sphere", "radius", 1.0F, 2.0F),
        movingQuadric(scene, "cone", "height", 1.0F, 3.0F),
        movingQuadric(scene, "cylinder", "zmax", 1.0F, 2.5F),
        movingQuadric(scene, "paraboloid", "rmax", 1.0F, 1.5F),
        movingQuadric(scene, "disk", "radius", 0.5F, 1.5F),
        movingQuadric(scene, "torus", "minorRadius", 0.2F, 0.5F),
    };
    const scene::Handle hyperboloid = scene.create("hyperboloid", "hyperboloid");
    scene.set(hyperboloid, "point2", scene::Value::point(glm::vec3 { 0.3F, 1.2F, 1.5F }));
    prototypes.push_back(hyperboloid);
    for (const char * profile : { "round", "flat" }) {
        const scene::Handle cloud = scene.create("points", std::string("cloud ") + profile);
        std::vector<glm::vec3> start;
        std::vector<glm::vec3> finish;
        std::vector<float> widths;
        for (int point = 0; point < 60; ++point) {
            start.push_back(glm::vec3 { next(seed), next(seed), next(seed) } * 10.0F);
            finish.push_back(start.back() + (glm::vec3 { next(seed), next(seed), next(seed) } - 0.5F) * 3.0F);
            widths.push_back(0.2F + next(seed));
        }
        scene.set(cloud, "P", { 0.0F, 1.0F }, { scene::Value::points(start), scene::Value::points(finish) });
        scene.set(cloud, "width", scene::Value::floats(widths));
        scene.set(cloud, "profile", scene::Value::string(profile));
        prototypes.push_back(cloud);
    }
    std::uint32_t count = 0;
    for (const scene::Handle & prototype : prototypes) {
        for (int copy = 0; copy < 3; ++copy) {
            const glm::mat4 place = glm::translate(glm::mat4(1.0F), glm::vec3 { next(seed), next(seed), next(seed) } * 15.0F) *
                glm::rotate(glm::mat4(1.0F), next(seed) * 6.0F, glm::normalize(glm::vec3 { next(seed), next(seed), next(seed) } + 0.1F)) *
                glm::scale(glm::mat4(1.0F), glm::vec3 { 0.5F + next(seed), 0.5F + (2.0F * next(seed)), 1.0F });
            const std::string name = "copy " + std::to_string(count++);
            if (copy == 2) {
                instanced(scene, name, prototype, { 0.0F, 1.0F }, { place, glm::translate(place, glm::vec3 { 2.0F, 0.0F, 1.0F }) });
            } else {
                instanced(scene, name, prototype, {}, { place });
            }
        }
    }
    scene.commit();
    scene.apply();
    const geometry::Prototypes gathered = geometry::gather(scene, {});
    BOOST_TEST(gathered.reports.empty());

    const std::vector<float> times { 0.0F, 0.2F, 0.45F, 0.7F, 1.0F };
    std::vector<kernel::geometry::Ray> rays;
    for (std::size_t i = 0; i < 3000; ++i) {
        kernel::geometry::Ray ray;
        ray.origin = glm::vec3 { next(seed), next(seed), next(seed) } * 30.0F - 5.0F;
        ray.direction = glm::vec3 { next(seed), next(seed), next(seed) } * 2.0F - 1.0F;
        ray.time = times[i % times.size()];
        rays.push_back(ray);
    }
    CpuDevice device;
    const view::Traced reference = view::trace(device, gathered, rays, view::Tracer::BruteForce);
    std::size_t hits = 0;
    for (const kernel::geometry::Hit & hit : reference.nearest) {
        hits += hit.found() ? 1 : 0;
    }
    // the scene is hit often enough that agreeing says something
    BOOST_TEST(hits > rays.size() / 10);
    for (const cpu::Isa isa : runnable()) {
        CpuDevice on(0, isa);
        const view::Traced traced = view::trace(on, gathered, rays, view::Tracer::Hierarchy);
        std::size_t disagreements = 0;
        for (std::size_t i = 0; i < rays.size(); ++i) {
            if (!agrees(reference.nearest[i], traced.nearest[i]) || reference.any[i] != traced.any[i]) {
                ++disagreements;
            }
        }
        BOOST_TEST(disagreements == 0U, v3d::moya::device::isaName(isa) << ": " << disagreements);
    }
}

// the hit view of a sphere straight ahead of the camera faces the camera at its middle, and the
// sphere's normal there comes back along the view
BOOST_AUTO_TEST_CASE(moya_shapes_the_hit_view_gives_a_quadrics_normal_test) {
    constexpr std::int32_t kSide = 15;
    scene::Scene scene(scene::kinds());
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "resolution", scene::Value::integers({ kSide, kSide }));
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(camera, "projection", scene::Value::string("perspective"));
    const scene::Handle ball = scene.create("sphere", "ball");
    instanced(scene, "ball", ball, {}, { glm::translate(glm::mat4(1.0F), glm::vec3 { 0.0F, 0.0F, 5.0F }) });
    scene.commit();
    scene.apply();

    for (const view::Tracer tracer : { view::Tracer::BruteForce, view::Tracer::Hierarchy }) {
        CpuDevice device;
        const v3d::moya::film::Snapshot snapshot = view::hits(device, scene, {}, 0.0F, tracer);
        const std::uint32_t middle = kSide / 2;
        BOOST_TEST(std::fabs(snapshot.at(middle, middle, snapshot.offset(view::kDepth)) - 4.0F) < 1.0e-4F);
        BOOST_TEST(std::fabs(snapshot.at(middle, middle, snapshot.offset(view::kNormal) + 2) + 1.0F) < 1.0e-4F);
        BOOST_TEST(snapshot.at(0, 0, snapshot.offset(view::kCoverage)) == 0.0F);
    }
}

};  // namespace

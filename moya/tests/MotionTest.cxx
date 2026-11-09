/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Builder.h>
#include <moya/libmoya/accel/Options.h>
#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/device/cpu/Isa.h>
#include <moya/libmoya/device/CpuLibrary.h>
#include <moya/libmoya/geometry/Motion.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Key.h>
#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Sampled.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Samples.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/view/Traced.h>
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
#include <glm/vec4.hpp>

namespace {

namespace accel = v3d::moya::accel;
namespace cpu = v3d::moya::device::cpu;
namespace geometry = v3d::moya::geometry;
namespace kernel = v3d::moya::kernel;
namespace scene = v3d::moya::scene;
namespace view = v3d::moya::view;
typedef v3d::moya::device::CpuDevice CpuDevice;

/** A random number in [0, 1) from a seed it moves on, the same on every compiler. **/
float next(std::uint32_t & seed) {
    seed = (seed * 1664525U) + 1013904223U;
    return static_cast<float>(seed >> 8U) / static_cast<float>(1U << 24U);
}

/** The scene's camera, whose shutter opens at 0 and closes at 1. **/
void shutter(scene::Scene & scene) {
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(camera, "shutter", scene::Value::floats({ 0.0F, 1.0F }));
}

/** A mesh of triangles of three points each, a face each, at each of its times. **/
scene::Handle mesh(scene::Scene & scene, const std::string & name, const std::vector<float> & times,
    const std::vector<std::vector<glm::vec3>> & samples) {
    const scene::Handle out = scene.create("mesh", name);
    std::vector<std::int32_t> indices;
    indices.reserve(samples.front().size());
    for (std::size_t i = 0; i < samples.front().size(); ++i) {
        indices.push_back(static_cast<std::int32_t>(i));
    }
    std::vector<scene::Value> values;
    values.reserve(samples.size());
    for (const std::vector<glm::vec3> & points : samples) {
        values.push_back(scene::Value::points(points));
    }
    if (times.empty()) {
        scene.set(out, "P", values.front());
    } else {
        scene.set(out, "P", times, values);
    }
    scene.set(out, "vertices", scene::Value::integers(std::vector<std::int32_t>(samples.front().size() / 3, 3)));
    scene.set(out, "indices", scene::Value::integers(indices));
    return out;
}

/** An instance of a prototype, in a group or the world, with a transformation at each time. **/
scene::Handle instanced(scene::Scene & scene, const std::string & name, const scene::Handle & prototype, const scene::Handle & group,
    const std::vector<float> & times, const std::vector<glm::mat4> & transforms) {
    const scene::Handle instance = scene.create("instance", name);
    scene.set(instance, "prototype", scene::Value::handle(prototype));
    if (group.valid()) {
        scene.set(instance, "group", scene::Value::handle(group));
    }
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

/** A ray down -z from z = 50 through a point, at a time. **/
kernel::geometry::Ray down(float x, float y, float time) {  // NOLINT(bugprone-easily-swappable-parameters)
    kernel::geometry::Ray ray;
    ray.origin = glm::vec3 { x, y, 50.0F };
    ray.direction = glm::vec3 { 0.0F, 0.0F, -1.0F };
    ray.time = time;
    return ray;
}

glm::mat4 moved(float x, float y, float z) {  // NOLINT(bugprone-easily-swappable-parameters)
    return glm::translate(glm::mat4(1.0F), glm::vec3 { x, y, z });
}

glm::mat4 turned(float degrees) {
    return glm::rotate(glm::mat4(1.0F), glm::radians(degrees), glm::vec3 { 0.0F, 0.0F, 1.0F });
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

/** A thin bar along +x from the origin to x = 10, in the plane z = 0. **/
std::vector<glm::vec3> bar() {
    return { { 0.0F, -0.1F, 0.0F }, { 10.0F, -0.1F, 0.0F }, { 10.0F, 0.1F, 0.0F }, { 0.0F, -0.1F, 0.0F }, { 10.0F, 0.1F, 0.0F },
        { 0.0F, 0.1F, 0.0F } };
}

// a transformation decomposed into a translation, a rotation and a scale composes back to itself,
// a mirror and a shear included
BOOST_AUTO_TEST_CASE(moya_motion_a_decomposed_transformation_composes_back_test) {
    std::uint32_t seed = 3;
    for (int trial = 0; trial < 50; ++trial) {
        glm::mat4 transform = moved(next(seed) * 20.0F, next(seed) * 20.0F, next(seed) * 20.0F) *
            glm::rotate(glm::mat4(1.0F), next(seed) * 6.0F, glm::normalize(glm::vec3 { next(seed), next(seed), next(seed) } + 0.1F));
        transform[1][0] += next(seed) - 0.5F;
        transform = transform * glm::scale(glm::mat4(1.0F), glm::vec3 { 0.5F + next(seed), trial % 5 == 0 ? -1.5F : 1.5F, 2.0F });
        const glm::mat4 back = kernel::geometry::compose(geometry::decompose(transform, 0.0F));
        float error = 0.0F;
        for (int column = 0; column < 4; ++column) {
            error = std::max(error, glm::length(back[column] - transform[column]));
        }
        BOOST_TEST(error < 1.0e-4F, "trial " << trial);
    }
}

/** How many of a box's corners, at 401 times over a range, lie outside its sweep along keys. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the range's begin, then its end
std::size_t outsideSwept(const accel::Box & box, const std::vector<kernel::geometry::Key> & keys, float begin, float end) {
    const accel::LinearBox swept = accel::sweepBox(box, keys, begin, end);
    std::size_t out = 0;
    for (int step = 0; step <= 400; ++step) {
        const float along = static_cast<float>(step) / 400.0F;
        const glm::mat4 transform = kernel::geometry::transformAt(keys.data(), static_cast<std::uint32_t>(keys.size()), begin + ((end - begin) * along));
        const accel::Box held = swept.at(along);
        const glm::vec3 slack { 1.0e-4F };
        for (int corner = 0; corner < 8; ++corner) {
            const glm::vec3 point { (corner & 1) != 0 ? box.upper.x : box.lower.x, (corner & 2) != 0 ? box.upper.y : box.lower.y,
                (corner & 4) != 0 ? box.upper.z : box.lower.z };
            const glm::vec3 placed = glm::vec3(transform * glm::vec4(point, 1.0F));
            if (glm::any(glm::lessThan(placed, held.lower - slack)) || glm::any(glm::greaterThan(placed, held.upper + slack))) {
                ++out;
            }
        }
    }
    return out;
}

// a box swept along a transformation that turns and moves between three keys holds each of its
// corners, as the traversal moves them, at every time of the range
BOOST_AUTO_TEST_CASE(moya_motion_a_swept_box_holds_its_corners_at_every_time_test) {
    std::uint32_t seed = 11;
    for (int trial = 0; trial < 20; ++trial) {
        scene::Samples samples;
        samples.times = { 0.0F, 0.4F, 1.0F };
        for (int key = 0; key < 3; ++key) {
            const glm::vec3 axis = glm::normalize(glm::vec3 { next(seed), next(seed), next(seed) } - 0.4F);
            samples.values.push_back(scene::Value::matrix(moved(next(seed) * 10.0F, next(seed) * 10.0F, 0.0F) *
                glm::rotate(glm::mat4(1.0F), next(seed) * 3.0F, axis) * glm::scale(glm::mat4(1.0F), glm::vec3 { 0.5F + next(seed) })));
        }
        const std::vector<kernel::geometry::Key> keys = geometry::keys(samples);
        const accel::Box box { .lower = { -1.0F, -2.0F, -0.5F }, .upper = { 3.0F, 1.0F, 0.5F } };
        const float begin = trial % 2 == 0 ? 0.0F : 0.2F;
        const float end = trial % 2 == 0 ? 1.0F : 0.7F;
        BOOST_TEST(outsideSwept(box, keys, begin, end) == 0U, "trial " << trial);
    }
}

// a bar turned a quarter turn about z over the shutter lies along the diagonal at its middle. A
// ray through the far half of the diagonal hits it then, where boxes interpolated between the
// bar's ends would not reach, and misses it when the shutter opens. A ray after the shutter closes
// meets the bar as it stands at the close
BOOST_AUTO_TEST_CASE(moya_motion_a_rotating_instance_is_hit_at_the_middle_of_the_shutter_test) {
    scene::Scene scene(scene::kinds());
    shutter(scene);
    const scene::Handle spinner = instanced(scene, "spinner", mesh(scene, "bar", {}, { bar() }), {}, { 0.0F, 1.0F }, { turned(0.0F), turned(90.0F) });
    scene.commit();
    scene.apply();

    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(prototypes.instances.front().moves());
    eachTraversal([&](view::Tracer tracer, cpu::Isa isa) {
        CpuDevice on(0, isa);
        const view::Traced traced = view::trace(on, prototypes, { down(6.5F, 6.5F, 0.5F), down(6.5F, 6.5F, 0.0F), down(0.0F, 8.0F, 1.0F), down(0.0F, 8.0F, 2.0F) }, tracer);
        const std::string name = tracerName(tracer, isa);
        BOOST_TEST(traced.nearest[0].found(), name);
        BOOST_TEST(traced.nearest[0].instances[0] == spinner.index, name);
        BOOST_TEST(std::fabs(traced.nearest[0].distance - 50.0F) < 1.0e-4F, name);
        BOOST_TEST(!traced.any[1], name);
        BOOST_TEST(traced.any[2], name);
        BOOST_TEST(traced.nearest[3].found(), name);
        BOOST_TEST(traced.nearest[3].time == 2.0F, name);
    });
}

// a triangle at z = 0, then z = 4 at the middle of the shutter, then z = 0 again bends through
// its middle sample, and lies between its samples where a ray meets it between them
BOOST_AUTO_TEST_CASE(moya_motion_a_mesh_at_three_times_bends_through_its_middle_sample_test) {
    scene::Scene scene(scene::kinds());
    shutter(scene);
    const std::vector<glm::vec3> flat { { -1.0F, -1.0F, 0.0F }, { 1.0F, -1.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } };
    std::vector<glm::vec3> raised = flat;
    for (glm::vec3 & point : raised) {
        point.z = 4.0F;
    }
    instanced(scene, "triangle", mesh(scene, "bending", { 0.0F, 0.5F, 1.0F }, { flat, raised, flat }), {}, {}, { glm::mat4(1.0F) });
    scene.commit();
    scene.apply();

    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(prototypes.geometries.front().moves());
    const std::vector<float> times { 0.0F, 0.25F, 0.5F, 0.75F, 1.0F };
    const std::vector<float> heights { 0.0F, 2.0F, 4.0F, 2.0F, 0.0F };
    std::vector<kernel::geometry::Ray> rays;
    rays.reserve(times.size());
    for (const float time : times) {
        rays.push_back(down(0.0F, 0.0F, time));
    }
    eachTraversal([&](view::Tracer tracer, cpu::Isa isa) {
        CpuDevice on(0, isa);
        const view::Traced traced = view::trace(on, prototypes, rays, tracer);
        for (std::size_t i = 0; i < times.size(); ++i) {
            BOOST_TEST(std::fabs(traced.nearest[i].distance - (50.0F - heights[i])) < 1.0e-4F, tracerName(tracer, isa) << ", time " << times[i]);
        }
    });
}

/** Small triangles scattered through a cube of a side, at one sample. **/
std::vector<glm::vec3> scattered(std::size_t count, float side, std::uint32_t & seed) {  // NOLINT(bugprone-easily-swappable-parameters)
    std::vector<glm::vec3> points;
    points.reserve(count * 3);
    for (std::size_t i = 0; i < count; ++i) {
        const glm::vec3 centre = glm::vec3 { next(seed), next(seed), next(seed) } * side;
        for (int corner = 0; corner < 3; ++corner) {
            points.push_back(centre + (glm::vec3 { next(seed), next(seed), next(seed) } * (side / 10.0F)));
        }
    }
    return points;
}

/** The same points, each moved by up to a reach. **/
std::vector<glm::vec3> jostled(const std::vector<glm::vec3> & points, float reach, std::uint32_t & seed) {
    std::vector<glm::vec3> out = points;
    for (glm::vec3 & point : out) {
        point += (glm::vec3 { next(seed), next(seed), next(seed) } - 0.5F) * reach;
    }
    return out;
}

/** A turn about a random axis, a scale from a half to two, and a move of up to a reach. **/
glm::mat4 randomly(float reach, std::uint32_t & seed) {
    const glm::vec3 axis = glm::normalize(glm::vec3 { next(seed), next(seed), next(seed) } - 0.5F);
    const glm::mat4 turn = glm::rotate(glm::mat4(1.0F), next(seed) * 6.28F, axis);
    const glm::mat4 scale = glm::scale(glm::mat4(1.0F), glm::vec3 { 0.5F + (next(seed) * 1.5F) });
    return moved(next(seed) * reach, next(seed) * reach, next(seed) * reach) * turn * scale;
}

/**
 * Whether two traversals give one answer: both miss, or both meet the same primitive through the
 * same chain at distances within a thousandth. The oracle's triangles are moved into the world
 * and the hierarchy's rays into each instance, so the two round apart.
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

// a scene of groups in groups has instances that turn and move between their own keys, and meshes
// that bend between their own samples. At each of several times, the hierarchy hits it as the
// brute-force traversal hits the scene placed at that time, at every instruction set
BOOST_AUTO_TEST_CASE(moya_motion_a_moving_scene_is_hit_as_the_still_scene_at_each_time_test) {
    std::uint32_t seed = 29;
    scene::Scene scene(scene::kinds());
    shutter(scene);
    const std::vector<glm::vec3> many = scattered(400, 30.0F, seed);
    const scene::Handle still = mesh(scene, "still", {}, { many });
    const scene::Handle bending = mesh(scene, "bending", { 0.0F, 1.0F }, { many, jostled(many, 6.0F, seed) });
    const std::vector<glm::vec3> few = scattered(20, 10.0F, seed);
    const scene::Handle wobbling = mesh(scene, "wobbling", { 0.1F, 0.3F, 0.8F }, { few, jostled(few, 3.0F, seed), jostled(few, 3.0F, seed) });
    const scene::Handle inner = scene.create("group", "inner");
    const scene::Handle outer = scene.create("group", "outer");
    instanced(scene, "still in inner", still, inner, {}, { randomly(20.0F, seed) });
    instanced(scene, "bending in inner", bending, inner, {}, { randomly(20.0F, seed) });
    instanced(scene, "turning in inner", wobbling, inner, { 0.0F, 0.5F, 1.0F }, { randomly(20.0F, seed), randomly(20.0F, seed), randomly(20.0F, seed) });
    instanced(scene, "inner in outer", inner, outer, { 0.2F, 0.9F }, { randomly(30.0F, seed), randomly(30.0F, seed) });
    instanced(scene, "still in outer", still, outer, {}, { randomly(30.0F, seed) });
    std::uint32_t count = 0;
    for (const scene::Handle & prototype : { outer, outer, inner, still, bending, wobbling }) {
        const std::string name = "top " + std::to_string(count++);
        if (count % 2 == 0) {
            instanced(scene, name, prototype, {}, { 0.0F, 1.0F }, { randomly(60.0F, seed), randomly(60.0F, seed) });
        } else {
            instanced(scene, name, prototype, {}, {}, { randomly(60.0F, seed) });
        }
    }
    scene.commit();
    scene.apply();
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(prototypes.reports.empty());

    // a handful of times, so the brute-force traversal places the scene at each once
    const std::vector<float> times { 0.0F, 0.15F, 0.3F, 0.5F, 0.65F, 0.8F, 1.0F };
    std::vector<kernel::geometry::Ray> rays;
    for (std::size_t i = 0; i < 2100; ++i) {
        kernel::geometry::Ray ray;
        ray.origin = glm::vec3 { next(seed), next(seed), next(seed) } * 140.0F - 20.0F;
        ray.direction = glm::vec3 { next(seed), next(seed), next(seed) } * 2.0F - 1.0F;
        ray.time = times[i % times.size()];
        if (i % 5 == 0) {
            ray.end = next(seed) * 60.0F;
        }
        rays.push_back(ray);
    }
    CpuDevice device;
    const view::Traced reference = view::trace(device, prototypes, rays, view::Tracer::BruteForce);
    std::size_t hits = 0;
    for (const kernel::geometry::Hit & hit : reference.nearest) {
        hits += hit.found() ? 1 : 0;
    }
    // the scene is hit often enough that agreeing says something
    BOOST_TEST(hits > rays.size() / 10);
    for (const cpu::Isa isa : runnable()) {
        CpuDevice on(0, isa);
        const view::Traced traced = view::trace(on, prototypes, rays, view::Tracer::Hierarchy);
        std::size_t disagreements = 0;
        for (std::size_t i = 0; i < rays.size(); ++i) {
            if (!agrees(reference.nearest[i], traced.nearest[i]) || reference.any[i] != traced.any[i]) {
                ++disagreements;
            }
        }
        BOOST_TEST(disagreements == 0U, v3d::moya::device::isaName(isa) << ": " << disagreements);
    }
}

// triangles that each move far and come back over the shutter cost less to traverse when the
// builder may split a node by time. Bounds over either half hold them closely, and bounds over the
// whole shutter do not
BOOST_AUTO_TEST_CASE(moya_motion_splitting_by_time_lowers_the_cost_of_motion_that_turns_back_test) {
    std::uint32_t seed = 5;
    geometry::Prototypes prototypes;
    const std::vector<glm::vec3> start = scattered(500, 100.0F, seed);
    const std::vector<glm::vec3> away = jostled(start, 80.0F, seed);
    prototypes.points = start;
    prototypes.points.insert(prototypes.points.end(), away.begin(), away.end());
    prototypes.points.insert(prototypes.points.end(), start.begin(), start.end());
    prototypes.times = { 0.0F, 0.5F, 1.0F };
    const kernel::geometry::Sampled held { .first = 0, .count = static_cast<std::uint32_t>(start.size()), .times = 0, .samples = 3 };
    CpuDevice device;
    const accel::Sweep sweep = accel::triangleSweep(held, prototypes.points, prototypes.times);
    accel::Options without;
    without.temporal = false;
    const double split = accel::cost(accel::buildMoving(device, 500, sweep, 0.0F, 1.0F));
    const double unsplit = accel::cost(accel::buildMoving(device, 500, sweep, 0.0F, 1.0F, without));
    BOOST_TEST(split < unsplit * 0.8, split << " against " << unsplit);
}

};  // namespace

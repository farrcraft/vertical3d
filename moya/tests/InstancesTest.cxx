/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/Assembly.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/device/cpu/Isa.h>
#include <moya/libmoya/device/CpuLibrary.h>
#include <moya/libmoya/geometry/Flat.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/World.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
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

#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace cpu = v3d::moya::device::cpu;
namespace geometry = v3d::moya::geometry;
namespace kernel = v3d::moya::kernel;
namespace scene = v3d::moya::scene;
namespace view = v3d::moya::view;
typedef v3d::moya::device::CpuDevice CpuDevice;

/** One triangle with its right angle at the origin, in the plane z = 0. **/
std::vector<glm::vec3> piece() {
    return { { 0.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } };
}

/** A ray down -z from z = 50 through a point. **/
kernel::geometry::Ray down(float x, float y, std::uint32_t kind = kernel::geometry::kCameraRay) {  // NOLINT(bugprone-easily-swappable-parameters)
    kernel::geometry::Ray ray;
    ray.origin = glm::vec3 { x, y, 50.0F };
    ray.direction = glm::vec3 { 0.0F, 0.0F, -1.0F };
    ray.kind = kind;
    return ray;
}

// a mesh placed by two instances is one prototype and one hierarchy, and is hit in both places,
// each hit naming its own instance
BOOST_AUTO_TEST_CASE(moya_instances_a_prototype_instanced_twice_is_one_hierarchy_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle triangle = mesh(scene, "piece", piece());
    const scene::Handle left = instanced(scene, "left", triangle, {}, moved(-10.0F, 0.0F, 0.0F));
    const scene::Handle right = instanced(scene, "right", triangle, {}, moved(10.0F, 0.0F, 0.0F));
    scene.commit();
    scene.apply();

    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(prototypes.geometries.size() == 1U);
    BOOST_TEST(prototypes.points.size() == 3U);
    BOOST_TEST(prototypes.instances.size() == 2U);
    CpuDevice device;
    const v3d::moya::accel::Assembly assembly = v3d::moya::accel::assemble(device, prototypes);
    BOOST_TEST(assembly.geometries.size() == 1U);
    BOOST_TEST(assembly.instances.size() == 2U);
    BOOST_TEST(assembly.instances[0].root == assembly.instances[1].root);

    eachTraversal([&](view::Tracer tracer, cpu::Isa isa) {
        CpuDevice on(0, isa);
        const view::Traced traced = view::trace(on, prototypes, { down(-9.75F, 0.25F), down(10.25F, 0.25F), down(0.25F, 0.25F) }, tracer);
        const std::string name = tracerName(tracer, isa);
        BOOST_TEST(traced.nearest[0].found(), name);
        BOOST_TEST(traced.nearest[0].instances[0] == left.index, name);
        BOOST_TEST(traced.nearest[1].instances[0] == right.index, name);
        BOOST_TEST(traced.nearest[0].depth == 1U, name);
        BOOST_TEST(traced.nearest[1].primitive == 0U, name);
        BOOST_TEST(traced.nearest[1].distance == 50.0F, name);
        BOOST_TEST(!traced.nearest[2].found(), name);
        BOOST_TEST(!traced.any[2], name);
    });
}

// a mesh turned in a group, scaled in a group that holds that, and placed twice in the world is
// hit where all three transformations put it, each hit naming its chain outermost first
BOOST_AUTO_TEST_CASE(moya_instances_a_group_in_a_group_places_its_geometry_at_every_transformation_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle triangle = mesh(scene, "piece", piece());
    const scene::Handle inner = scene.create("group", "inner");
    const scene::Handle outer = scene.create("group", "outer");
    // a quarter turn about z takes the triangle to x <= 0, and then up to z = 1
    const scene::Handle turned = instanced(scene, "turned", triangle, inner,
        moved(0.0F, 0.0F, 1.0F) * glm::rotate(glm::mat4(1.0F), glm::radians(90.0F), glm::vec3 { 0.0F, 0.0F, 1.0F }));
    // twice the size takes it to z = 2, two on a side
    const scene::Handle doubled = instanced(scene, "doubled", inner, outer, glm::scale(glm::mat4(1.0F), glm::vec3 { 2.0F }));
    const scene::Handle left = instanced(scene, "left", outer, {}, moved(-10.0F, 0.0F, 0.0F));
    const scene::Handle right = instanced(scene, "right", outer, {}, moved(10.0F, 0.0F, 0.0F));
    scene.commit();
    scene.apply();

    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(prototypes.reports.empty());
    BOOST_TEST(prototypes.geometries.size() == 1U);
    BOOST_TEST(prototypes.groups.size() == 2U);
    const std::vector<kernel::geometry::Ray> rays { down(-11.0F, 0.5F), down(9.0F, 0.5F), down(-9.0F, 0.5F), down(-11.0F, 1.5F) };
    eachTraversal([&](view::Tracer tracer, cpu::Isa isa) {
        CpuDevice on(0, isa);
        const view::Traced traced = view::trace(on, prototypes, rays, tracer);
        const std::string name = tracerName(tracer, isa);
        for (std::size_t i = 0; i < 2; ++i) {
            const kernel::geometry::Hit & hit = traced.nearest[i];
            BOOST_TEST(hit.found(), name << ", ray " << i);
            BOOST_TEST(std::fabs(hit.distance - 48.0F) < 1.0e-4F, name << ", ray " << i);
            BOOST_TEST(hit.depth == 3U, name << ", ray " << i);
            BOOST_TEST(hit.instances[0] == (i == 0 ? left.index : right.index), name << ", ray " << i);
            BOOST_TEST(hit.instances[1] == doubled.index, name << ", ray " << i);
            BOOST_TEST(hit.instances[2] == turned.index, name << ", ray " << i);
        }
        // where the triangle would be without its turn, and beyond its scaled hypotenuse
        BOOST_TEST(!traced.nearest[2].found(), name);
        BOOST_TEST(!traced.nearest[3].found(), name);
    });
}

// a group that holds an instance of itself leaves that instance out, and a ray through what it
// places otherwise meets it once, inside the chain that fits
BOOST_AUTO_TEST_CASE(moya_instances_a_group_that_holds_itself_is_left_out_of_the_hierarchy_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle group = scene.create("group", "loop");
    const scene::Handle triangle = mesh(scene, "piece", piece());
    const scene::Handle held = instanced(scene, "held", triangle, group, glm::mat4(1.0F));
    instanced(scene, "again", group, group, moved(0.0F, 0.0F, 1.0F));
    const scene::Handle outer = instanced(scene, "outer", group, {}, glm::mat4(1.0F));
    scene.commit();
    scene.apply();

    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(reported(prototypes, "group loop holds itself"));
    BOOST_TEST(prototypes.reports.size() == 1U);
    BOOST_TEST(prototypes.groups.size() == 1U);
    BOOST_TEST(prototypes.groups[0].instances.size() == 1U);
    eachTraversal([&](view::Tracer tracer, cpu::Isa isa) {
        CpuDevice on(0, isa);
        const view::Traced traced = view::trace(on, prototypes, { down(0.25F, 0.25F) }, tracer);
        const std::string name = tracerName(tracer, isa);
        BOOST_TEST(traced.nearest[0].distance == 50.0F, name);
        BOOST_TEST(traced.nearest[0].depth == 2U, name);
        BOOST_TEST(traced.nearest[0].instances[0] == outer.index, name);
        BOOST_TEST(traced.nearest[0].instances[1] == held.index, name);
    });
}

// of two groups that hold each other, the world reaches one, and its instance of the other is
// left out, since that one holds it
BOOST_AUTO_TEST_CASE(moya_instances_two_groups_that_hold_each_other_are_reported_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle first = scene.create("group", "first");
    const scene::Handle second = scene.create("group", "second");
    const scene::Handle triangle = mesh(scene, "piece", piece());
    instanced(scene, "in first", triangle, first, glm::mat4(1.0F));
    instanced(scene, "in second", triangle, second, moved(5.0F, 0.0F, 0.0F));
    instanced(scene, "second in first", second, first, glm::mat4(1.0F));
    instanced(scene, "first in second", first, second, glm::mat4(1.0F));
    instanced(scene, "top", first, {}, glm::mat4(1.0F));
    scene.commit();
    scene.apply();

    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(reported(prototypes, "group second holds itself"));
    BOOST_TEST(prototypes.reports.size() == 1U);
    const geometry::Flat flat = geometry::flatten(prototypes);
    BOOST_TEST(flat.placements.size() == 1U);
    BOOST_TEST(flat.placements[0].depth == 2U);
}

// groups nested one deeper than a hit records are left out by the hierarchy as by the oracle, and
// the chain that fits is still hit
BOOST_AUTO_TEST_CASE(moya_instances_a_chain_deeper_than_a_hit_records_is_not_traversed_test) {
    scene::Scene scene(scene::kinds());
    std::vector<scene::Handle> groups;
    groups.reserve(kernel::geometry::kInstanceDepth);
    for (std::uint32_t level = 0; level < kernel::geometry::kInstanceDepth; ++level) {
        groups.push_back(scene.create("group", "level " + std::to_string(level)));
    }
    // the deepest group holds the mesh, each group holds the one below it, and the world the top,
    // moved aside so that the chain too deep would be hit elsewhere
    instanced(scene, "held", mesh(scene, "piece", piece()), groups.back(), glm::mat4(1.0F));
    for (std::size_t level = 0; level + 1 < groups.size(); ++level) {
        instanced(scene, "into " + std::to_string(level + 1), groups[level + 1], groups[level], glm::mat4(1.0F));
    }
    instanced(scene, "top", groups.front(), {}, moved(5.0F, 0.0F, 0.0F));
    const scene::Handle shorter = instanced(scene, "short", groups.back(), {}, glm::mat4(1.0F));
    scene.commit();
    scene.apply();

    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(reported(prototypes, "instance held is nested deeper than"));
    BOOST_TEST(prototypes.reports.size() == 1U);
    eachTraversal([&](view::Tracer tracer, cpu::Isa isa) {
        CpuDevice on(0, isa);
        const view::Traced traced = view::trace(on, prototypes, { down(0.25F, 0.25F), down(5.25F, 0.25F) }, tracer);
        const std::string name = tracerName(tracer, isa);
        BOOST_TEST(traced.nearest[0].depth == 2U, name);
        BOOST_TEST(traced.nearest[0].instances[0] == shorter.index, name);
        BOOST_TEST(!traced.nearest[1].found(), name);
        BOOST_TEST(!traced.any[1], name);
    });
}

// an instance hidden from shadow rays casts no shadow and is still seen, and a group's instance
// hidden from camera rays hides everything in the group from them
BOOST_AUTO_TEST_CASE(moya_instances_an_instance_hidden_from_shadows_casts_none_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle triangle = mesh(scene, "piece", piece());
    const scene::Handle unshadowed = instanced(scene, "unshadowed", triangle, {}, glm::mat4(1.0F));
    scene.set(unshadowed, "shadow", scene::Value::integer(0));
    const scene::Handle group = scene.create("group", "pair");
    instanced(scene, "held", triangle, group, glm::mat4(1.0F));
    const scene::Handle unseen = instanced(scene, "unseen", group, {}, moved(5.0F, 0.0F, 0.0F));
    scene.set(unseen, "camera", scene::Value::integer(0));
    scene.commit();
    scene.apply();

    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    const std::vector<kernel::geometry::Ray> rays { down(0.25F, 0.25F, kernel::geometry::kShadowRay),
        down(0.25F, 0.25F, kernel::geometry::kCameraRay), down(0.25F, 0.25F, kernel::geometry::kIndirectRay),
        down(5.25F, 0.25F, kernel::geometry::kCameraRay), down(5.25F, 0.25F, kernel::geometry::kShadowRay) };
    eachTraversal([&](view::Tracer tracer, cpu::Isa isa) {
        CpuDevice on(0, isa);
        const view::Traced traced = view::trace(on, prototypes, rays, tracer);
        const std::string name = tracerName(tracer, isa);
        BOOST_TEST(!traced.any[0], name);
        BOOST_TEST(!traced.nearest[0].found(), name);
        BOOST_TEST(traced.any[1], name);
        BOOST_TEST(traced.nearest[1].instances[0] == unshadowed.index, name);
        BOOST_TEST(traced.any[2], name);
        BOOST_TEST(!traced.any[3], name);
        BOOST_TEST(!traced.nearest[3].found(), name);
        BOOST_TEST(traced.nearest[4].instances[0] == unseen.index, name);
    });
}

// a scene of groups in groups is turned, scaled and moved, and some instances are hidden from some
// kinds of ray. Over random rays of every kind, the hierarchy finds the brute-force traversal's
// nearest hit, at every instruction set this processor runs
BOOST_AUTO_TEST_CASE(moya_instances_the_hierarchy_finds_the_oracles_nearest_hit_through_nested_instances_test) {
    std::uint32_t seed = 17;
    scene::Scene scene(scene::kinds());
    const scene::Handle many = mesh(scene, "many", scattered(300, 20.0F, 2.0F, seed));
    const scene::Handle few = mesh(scene, "few", scattered(6, 10.0F, 1.0F, seed));
    const scene::Handle inner = scene.create("group", "inner");
    const scene::Handle outer = scene.create("group", "outer");
    for (int i = 0; i < 3; ++i) {
        instanced(scene, "many in inner " + std::to_string(i), many, inner, randomly(20.0F, seed));
    }
    instanced(scene, "few in inner", few, inner, randomly(20.0F, seed));
    for (int i = 0; i < 2; ++i) {
        instanced(scene, "inner in outer " + std::to_string(i), inner, outer, randomly(30.0F, seed));
    }
    instanced(scene, "many in outer", many, outer, randomly(30.0F, seed));
    std::uint32_t count = 0;
    for (const scene::Handle & prototype : { outer, outer, outer, inner, inner, many, few }) {
        const scene::Handle instance = instanced(scene, "top " + std::to_string(count), prototype, {}, randomly(60.0F, seed));
        // every third is hidden from one kind of ray
        if (count % 3 == 1) {
            scene.set(instance, count % 2 == 0 ? "shadow" : "camera", scene::Value::integer(0));
        }
        ++count;
    }
    scene.commit();
    scene.apply();
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(prototypes.reports.empty());

    std::vector<kernel::geometry::Ray> rays;
    for (std::size_t i = 0; i < 3000; ++i) {
        kernel::geometry::Ray ray;
        ray.origin = glm::vec3 { next(seed), next(seed), next(seed) } * 140.0F - 20.0F;
        ray.direction = glm::vec3 { next(seed), next(seed), next(seed) } * 2.0F - 1.0F;
        const std::uint32_t kinds[] = { kernel::geometry::kCameraRay, kernel::geometry::kShadowRay, kernel::geometry::kIndirectRay };
        ray.kind = kinds[i % 3];
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

};  // namespace
};  // namespace v3d::moya::tracing

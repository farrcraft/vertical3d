/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/device/cpu/Isa.h>
#include <moya/libmoya/device/CpuLibrary.h>
#include <moya/libmoya/geometry/Geometry.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/view/Traced.h>
#include <moya/libmoya/view/Tracing.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace cpu = v3d::moya::device::cpu;
namespace geometry = v3d::moya::geometry;
namespace kernel = v3d::moya::kernel;
namespace view = v3d::moya::view;
typedef v3d::moya::device::CpuDevice CpuDevice;

/** Triangles of three points each, as one mesh placed in the world by one instance. **/
geometry::Prototypes placed(const std::vector<glm::vec3> & points) {
    geometry::Prototypes prototypes;
    prototypes.points = points;
    const auto count = static_cast<std::uint32_t>(points.size() / 3);
    prototypes.faces.resize(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        prototypes.faces[i] = i;
    }
    prototypes.geometries.push_back(geometry::Geometry { .first = 0, .count = count, .points = { .first = 0, .count = count * 3 } });
    prototypes.instances.push_back(geometry::Instance { .id = 7, .visibility = kernel::geometry::kEveryRay });
    prototypes.world.instances.push_back(0);
    return prototypes;
}

/** Small triangles scattered through a cube of side 100, each up to 4 across. **/
std::vector<glm::vec3> scattered(std::size_t count, std::uint32_t seed) {  // NOLINT(bugprone-easily-swappable-parameters)
    return tracing::scattered(count, 100.0F, 4.0F, seed);
}

/** A closed box of side 100 in twelve triangles, which a ray from inside always hits. **/
std::vector<glm::vec3> closedBox() {
    const glm::vec3 corner[8] = { { 0, 0, 0 }, { 100, 0, 0 }, { 100, 100, 0 }, { 0, 100, 0 }, { 0, 0, 100 }, { 100, 0, 100 }, { 100, 100, 100 },
        { 0, 100, 100 } };
    const int faces[6][4] = { { 0, 1, 2, 3 }, { 4, 7, 6, 5 }, { 0, 4, 5, 1 }, { 1, 5, 6, 2 }, { 2, 6, 7, 3 }, { 3, 7, 4, 0 } };
    std::vector<glm::vec3> points;
    for (const auto & face : faces) {
        points.insert(points.end(), { corner[face[0]], corner[face[1]], corner[face[2]], corner[face[0]], corner[face[2]], corner[face[3]] });
    }
    return points;
}

/**
 * Rays from inside and outside the cube of side 100, in every direction, some with an end. A
 * third run along an axis, so the box test meets directions of zero.
 **/
std::vector<kernel::geometry::Ray> rays(std::size_t count, std::uint32_t seed) {  // NOLINT(bugprone-easily-swappable-parameters)
    std::vector<kernel::geometry::Ray> out;
    out.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        kernel::geometry::Ray ray;
        ray.origin = glm::vec3 { next(seed), next(seed), next(seed) } * 140.0F - 20.0F;
        if (i % 3 == 0) {
            glm::vec3 direction { 0.0F };
            direction[static_cast<int>(i / 3) % 3] = next(seed) < 0.5F ? -1.0F : 1.0F;
            ray.direction = direction;
        } else {
            ray.direction = glm::vec3 { next(seed), next(seed), next(seed) } * 2.0F - 1.0F;
        }
        if (i % 5 == 0) {
            ray.end = next(seed) * 60.0F;
        }
        out.push_back(ray);
    }
    return out;
}

/**
 * How many rays the hierarchy at an instruction set answers otherwise than the brute-force
 * traversal. An answer is a miss, or a hit on one primitive through one chain at one distance.
 * Both traverse one mesh placed by one instance that does not move it, so a hit's distance is the
 * same to the bit.
 **/
std::size_t disagreements(const geometry::Prototypes & prototypes, const std::vector<kernel::geometry::Ray> & list, cpu::Isa isa) {
    CpuDevice device(0, isa);
    const view::Traced reference = view::trace(device, prototypes, list, view::Tracer::BruteForce);
    const view::Traced traced = view::trace(device, prototypes, list, view::Tracer::Hierarchy);
    std::size_t out = 0;
    for (std::size_t i = 0; i < list.size(); ++i) {
        const kernel::geometry::Hit & want = reference.nearest[i];
        const kernel::geometry::Hit & got = traced.nearest[i];
        const bool same = want.found() == got.found() &&
            (!want.found() || (want.distance == got.distance && want.primitive == got.primitive && want.depth == got.depth &&
                std::equal(want.instances, want.instances + want.depth, got.instances)));
        if (!same || reference.any[i] != traced.any[i]) {
            ++out;
        }
    }
    return out;
}

// over random rays, from inside boxes and out, the hierarchy finds the brute-force traversal's
// nearest hit, at every instruction set this processor runs
BOOST_AUTO_TEST_CASE(moya_traversal_the_hierarchy_finds_the_oracles_nearest_hit_test) {
    const std::vector<std::pair<std::string, std::vector<glm::vec3>>> scenes { { "scattered", scattered(4000, 9) },
        { "slivers", slivers(300, 1.0F) }, { "closed box", closedBox() }, { "one triangle", scattered(1, 4) } };
    const std::vector<kernel::geometry::Ray> list = rays(3000, 21);
    for (const cpu::Isa isa : runnable()) {
        for (const auto & [name, points] : scenes) {
            BOOST_TEST(disagreements(placed(points), list, isa) == 0U, v3d::moya::device::isaName(isa) << ", " << name);
        }
    }
}

// a ray from inside a closed box always hits it, through the hierarchy as through the oracle
BOOST_AUTO_TEST_CASE(moya_traversal_a_ray_from_inside_a_closed_box_hits_it_test) {
    std::vector<kernel::geometry::Ray> inside;
    std::uint32_t seed = 3;
    for (int i = 0; i < 500; ++i) {
        kernel::geometry::Ray ray;
        ray.origin = glm::vec3 { next(seed), next(seed), next(seed) } * 98.0F + 1.0F;
        ray.direction = glm::vec3 { next(seed), next(seed), next(seed) } * 2.0F - 1.0F;
        inside.push_back(ray);
    }
    for (const cpu::Isa isa : runnable()) {
        CpuDevice device(0, isa);
        const view::Traced traced = view::trace(device, placed(closedBox()), inside, view::Tracer::Hierarchy);
        std::size_t missed = 0;
        for (std::size_t i = 0; i < inside.size(); ++i) {
            missed += traced.nearest[i].found() && traced.any[i] ? 0 : 1;
        }
        BOOST_TEST(missed == 0U, v3d::moya::device::isaName(isa));
    }
}

// a device asked for a newer instruction set than the processor runs takes the processor's, and
// each library it loads is the one it asked for
BOOST_AUTO_TEST_CASE(moya_traversal_a_device_takes_the_older_of_its_and_the_processors_test) {
    BOOST_TEST((CpuDevice(0, cpu::Isa::Avx512).isa() == v3d::moya::device::newestIsa()));
    BOOST_TEST((CpuDevice(0, cpu::Isa::Sse42).isa() == cpu::Isa::Sse42));
    for (const cpu::Isa isa : runnable()) {
        CpuDevice device(0, isa);
        BOOST_TEST((device.module().isa == isa), v3d::moya::device::isaName(isa));
        BOOST_TEST(v3d::moya::device::CpuLibrary::path(isa).find("moya_cpu_" + v3d::moya::device::isaName(isa)) != std::string::npos);
    }
}

/**
 * A library whose table names another instruction set than the one asked for is refused. An
 * instruction set past the last is named as SSE4.2 is, so its library is the SSE4.2 one, whose
 * table names SSE4.2.
 *
 * Covers the base: 067d6944 added the check to CpuLibrary's constructor. Checked by deleting its
 * comparison of the table's instruction set, which fails this case.
 **/
BOOST_AUTO_TEST_CASE(moya_traversal_a_library_for_another_instruction_set_is_refused_test) {
    const auto unknown = static_cast<cpu::Isa>(static_cast<std::uint8_t>(cpu::Isa::Avx512) + 1U);
    BOOST_TEST(v3d::moya::device::isaName(unknown) == "sse42");
    BOOST_CHECK_THROW(static_cast<void>(v3d::moya::device::CpuLibrary(unknown)), std::runtime_error);
    BOOST_CHECK_NO_THROW(static_cast<void>(v3d::moya::device::CpuLibrary(cpu::Isa::Sse42)));
}

};  // namespace
};  // namespace v3d::moya::tracing

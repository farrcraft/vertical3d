/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Builder.h>
#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/device/cpu/Node.h>
#include <moya/libmoya/accel/Triangles.h>
#include <moya/libmoya/device/CpuDevice.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <future>
#include <limits>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

namespace {

namespace accel = v3d::moya::accel;
namespace cpu = v3d::moya::device::cpu;
typedef v3d::moya::device::CpuDevice CpuDevice;

/** A random number in [0, 1) from a seed it moves on, the same on every compiler. **/
float next(std::uint32_t & seed) {
    seed = (seed * 1664525U) + 1013904223U;
    return static_cast<float>(seed >> 8U) / static_cast<float>(1U << 24U);
}

/** Small triangles scattered through a cube of side 100, three points each. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - how many, then the seed
std::vector<glm::vec3> scattered(std::size_t count, std::uint32_t seed) {
    std::vector<glm::vec3> points;
    points.reserve(count * 3);
    for (std::size_t i = 0; i < count; ++i) {
        const glm::vec3 centre { next(seed) * 100.0F, next(seed) * 100.0F, next(seed) * 100.0F };
        for (int corner = 0; corner < 3; ++corner) {
            points.push_back(centre + glm::vec3 { next(seed), next(seed), next(seed) });
        }
    }
    return points;
}

/**
 * Long thin triangles along the diagonal of a square of side 100, each moved along x from the one
 * before. Their boxes overlap far more than the triangles do.
 **/
std::vector<glm::vec3> slivers(std::size_t count) {
    std::vector<glm::vec3> points;
    points.reserve(count * 3);
    for (std::size_t i = 0; i < count; ++i) {
        const float offset = static_cast<float>(i) * (90.0F / static_cast<float>(count));
        points.emplace_back(offset, 0.0F, 0.0F);
        points.emplace_back(100.0F, 100.0F - offset, 0.0F);
        points.emplace_back(100.0F, 100.0F - offset, 0.01F);
    }
    return points;
}

accel::Box boxOf(const cpu::Node & node, std::uint32_t slot) {
    return accel::Box { .lower = { node.lowerX[slot], node.lowerY[slot], node.lowerZ[slot] },
        .upper = { node.upperX[slot], node.upperY[slot], node.upperZ[slot] } };
}

/**
 * Walks the hierarchy from a node inside a box, counting each primitive's references, and
 * checking that each child lies in its parent and each reference's primitive meets its leaf.
 **/
void walk(const accel::Hierarchy & hierarchy, const std::vector<accel::Box> & bounds, std::uint32_t at, const accel::Box & parent,
    std::vector<std::uint32_t> & seen, int & breaches) {
    const cpu::Node & node = hierarchy.nodes[at];
    for (std::uint32_t slot = 0; slot < cpu::kWidth; ++slot) {
        if (node.child[slot] == cpu::kEmpty) {
            continue;
        }
        const accel::Box box = boxOf(node, slot);
        // a little slack for boxes grown from clipped parts
        accel::Box slack = parent;
        slack.lower -= glm::vec3(1.0e-3F);
        slack.upper += glm::vec3(1.0e-3F);
        if (!slack.holds(box)) {
            ++breaches;
        }
        if ((node.child[slot] & cpu::kLeaf) != 0) {
            const std::uint32_t first = node.child[slot] & ~cpu::kLeaf;
            for (std::uint32_t i = 0; i < node.count[slot]; ++i) {
                const std::uint32_t primitive = hierarchy.references[first + i];
                ++seen[primitive];
                if (bounds[primitive].within(box).empty()) {
                    ++breaches;
                }
            }
        } else {
            walk(hierarchy, bounds, node.child[slot], box, seen, breaches);
        }
    }
}

BOOST_AUTO_TEST_CASE(moya_builder_every_primitive_is_reachable_from_the_root_test) {
    CpuDevice device;
    const std::vector<glm::vec3> points = scattered(3000, 7);
    const std::vector<accel::Box> bounds = accel::triangleBounds(points);
    for (const bool spatial : { false, true }) {
        const accel::Hierarchy hierarchy = accel::build(device, bounds, accel::triangleClip(points), { .spatial = spatial });
        std::vector<std::uint32_t> seen(bounds.size(), 0);
        int breaches = 0;
        walk(hierarchy, bounds, 0, hierarchy.bounds, seen, breaches);
        BOOST_TEST(breaches == 0, "spatial " << spatial);
        std::size_t missing = 0;
        std::size_t twice = 0;
        for (const std::uint32_t count : seen) {
            missing += count == 0 ? 1 : 0;
            twice += count > 1 ? 1 : 0;
        }
        BOOST_TEST(missing == 0U, "spatial " << spatial);
        // without spatial splits, each primitive is referenced once
        if (!spatial) {
            BOOST_TEST(twice == 0U);
            BOOST_TEST(hierarchy.references.size() == bounds.size());
        }
        // a budget of one allows at most twice the primitives
        BOOST_TEST(hierarchy.references.size() <= 2 * bounds.size());
    }
}

// triangles whose boxes overlap far more than the triangles do cost less with spatial splits
BOOST_AUTO_TEST_CASE(moya_builder_spatial_splits_lower_the_cost_of_long_thin_triangles_test) {
    CpuDevice device;
    const std::vector<glm::vec3> points = slivers(500);
    const std::vector<accel::Box> bounds = accel::triangleBounds(points);
    const double without = accel::cost(accel::build(device, bounds, accel::triangleClip(points), { .spatial = false }));
    const accel::Hierarchy split = accel::build(device, bounds, accel::triangleClip(points), { .spatial = true });
    const double with = accel::cost(split);
    BOOST_TEST(with < without * 0.8, "with " << with << ", without " << without);
    BOOST_TEST(split.references.size() > bounds.size());
}

// a scene large enough that its halves build on separate threads builds the same on one thread
BOOST_AUTO_TEST_CASE(moya_builder_two_builds_on_different_threads_are_equal_test) {
    const std::vector<glm::vec3> points = scattered(20000, 11);
    const std::vector<accel::Box> bounds = accel::triangleBounds(points);
    CpuDevice one(1);
    CpuDevice many(0);
    const accel::Hierarchy first = accel::build(one, bounds, accel::triangleClip(points));
    const accel::Hierarchy second = accel::build(many, bounds, accel::triangleClip(points));
    BOOST_TEST_REQUIRE(first.nodes.size() == second.nodes.size());
    BOOST_TEST(std::memcmp(first.nodes.data(), second.nodes.data(), first.nodes.size() * sizeof(cpu::Node)) == 0);
    BOOST_TEST(first.references == second.references);
    BOOST_TEST(first.nodes.size() > 1U);
}

BOOST_AUTO_TEST_CASE(moya_builder_no_primitives_and_one_primitive_each_have_a_root_test) {
    CpuDevice device;
    const accel::Hierarchy none = accel::build(device, {}, accel::triangleClip({}));
    BOOST_TEST(none.nodes.size() == 1U);
    for (std::uint32_t slot = 0; slot < cpu::kWidth; ++slot) {
        BOOST_TEST(none.nodes[0].child[slot] == cpu::kEmpty);
    }

    const std::vector<glm::vec3> points { { 0.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } };
    const accel::Hierarchy one = accel::build(device, accel::triangleBounds(points), accel::triangleClip(points));
    BOOST_TEST(one.nodes.size() == 1U);
    BOOST_TEST(one.nodes[0].child[0] == cpu::kLeaf);
    BOOST_TEST(one.nodes[0].count[0] == 1U);
    BOOST_TEST(one.nodes[0].child[1] == cpu::kEmpty);
}

// a node holds up to eight children, and a hierarchy of many primitives fills them
BOOST_AUTO_TEST_CASE(moya_builder_nodes_hold_up_to_eight_children_test) {
    CpuDevice device;
    const std::vector<glm::vec3> points = scattered(5000, 3);
    const accel::Hierarchy hierarchy = accel::build(device, accel::triangleBounds(points), accel::triangleClip(points));
    std::size_t children = 0;
    for (const cpu::Node & node : hierarchy.nodes) {
        bool packed = true;
        bool ended = false;
        for (std::uint32_t slot = 0; slot < cpu::kWidth; ++slot) {
            const bool empty = node.child[slot] == cpu::kEmpty;
            packed = packed && (!ended || empty);
            ended = ended || empty;
            children += empty ? 0 : 1;
        }
        BOOST_TEST(packed);
    }
    // a binary hierarchy would average two children a node, and leaves of up to eight keep the
    // nodes above them from filling every slot
    BOOST_TEST(static_cast<double>(children) / static_cast<double>(hierarchy.nodes.size()) > 3.0);
}

BOOST_AUTO_TEST_CASE(moya_builder_a_clipped_triangle_is_bounded_inside_the_box_test) {
    const glm::vec3 p0 { -1.0F, -1.0F, 0.0F };
    const glm::vec3 p1 { 3.0F, -1.0F, 0.0F };
    const glm::vec3 p2 { -1.0F, 3.0F, 0.0F };
    // the box's corner at (1, 1) lies on the triangle's long edge, x + y = 2
    const accel::Box box { .lower = { 0.0F, 0.0F, -1.0F }, .upper = { 2.0F, 2.0F, 1.0F } };
    const accel::Box clipped = accel::clipTriangle(p0, p1, p2, box);
    BOOST_TEST(clipped.lower.x == 0.0F);
    BOOST_TEST(clipped.lower.y == 0.0F);
    BOOST_TEST(clipped.upper.x == 2.0F);
    BOOST_TEST(clipped.upper.y == 2.0F);
    BOOST_TEST(clipped.lower.z == 0.0F);
    BOOST_TEST(clipped.upper.z == 0.0F);

    const accel::Box away { .lower = { 5.0F, 5.0F, -1.0F }, .upper = { 6.0F, 6.0F, 1.0F } };
    BOOST_TEST(accel::clipTriangle(p0, p1, p2, away).empty());
}

BOOST_AUTO_TEST_CASE(moya_builder_a_budget_that_is_not_a_count_is_refused_test) {
    CpuDevice device;
    const std::vector<glm::vec3> points = scattered(10, 5);
    const std::vector<accel::Box> bounds = accel::triangleBounds(points);
    for (const float budget : { -1.0F, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity() }) {
        BOOST_CHECK_THROW(accel::build(device, bounds, accel::triangleClip(points), { .budget = budget }), std::invalid_argument);
    }
}

// nine boxes that move together along x split by time only while that pays. A path splits so at
// most eight times, and its node is then halved in order, so each reference is held at most 256
// times. The build runs on a thread of its own, so a build that never ends fails the case rather
// than hanging the suite
BOOST_AUTO_TEST_CASE(moya_builder_primitives_moving_together_build_a_bounded_hierarchy_test) {
    std::promise<std::size_t> built;
    std::future<std::size_t> references = built.get_future();
    std::thread([built = std::move(built)]() mutable {
        try {
            CpuDevice device;
            const accel::Sweep sweep = [](std::uint32_t, float begin, float end) {
                const auto at = [](float time) {
                    return accel::Box { .lower = { 1000.0F * time, 0.0F, 0.0F }, .upper = { (1000.0F * time) + 1.0F, 1.0F, 1.0F } };
                };
                return accel::LinearBox { .start = at(begin), .end = at(end) };
            };
            built.set_value(accel::buildMoving(device, 9, sweep, 0.0F, 1.0F).references.size());
        } catch (...) {
            built.set_exception(std::current_exception());
        }
    }).detach();
    BOOST_TEST_REQUIRE((references.wait_for(std::chrono::seconds(30)) == std::future_status::ready));
    BOOST_TEST(references.get() <= 9U * 256U);
}

};  // namespace

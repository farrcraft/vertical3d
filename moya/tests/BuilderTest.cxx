/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Builder.h>
#include <moya/libmoya/accel/Hierarchy.h>
#include <moya/libmoya/accel/MovingHierarchy.h>
#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/device/cpu/Node.h>
#include <moya/libmoya/accel/Triangles.h>
#include <moya/libmoya/device/CpuDevice.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <future>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace accel = v3d::moya::accel;
namespace cpu = v3d::moya::device::cpu;
typedef v3d::moya::device::CpuDevice CpuDevice;

/** Small triangles scattered through a cube of side 100, each up to 1 across. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - how many, then the seed
std::vector<glm::vec3> scattered(std::size_t count, std::uint32_t seed) {
    return tracing::scattered(count, 100.0F, 1.0F, seed);
}

accel::Box boxOf(const cpu::Node & node, std::uint32_t slot) {
    return accel::Box { .lower = { node.lowerX[slot], node.lowerY[slot], node.lowerZ[slot] },
        .upper = { node.upperX[slot], node.upperY[slot], node.upperZ[slot] } };
}

/** The most references a leaf of a hierarchy holds, of still nodes or moving ones. **/
template <typename N>
std::uint32_t fullestLeaf(const std::vector<N> & nodes) {
    std::uint32_t out = 0;
    for (const N & node : nodes) {
        for (std::uint32_t slot = 0; slot < cpu::kWidth; ++slot) {
            if (node.child[slot] != cpu::kEmpty && (node.child[slot] & cpu::kLeaf) != 0) {
                out = std::max(out, node.count[slot]);
            }
        }
    }
    return out;
}

/** A hierarchy's references in order. **/
std::vector<std::uint32_t> sorted(std::vector<std::uint32_t> references) {
    std::ranges::sort(references);
    return references;
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
    const std::vector<glm::vec3> points = slivers(500, 0.01F);
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

/**
 * Forty triangles at one place have no split by centroids. Each build halves them in order until
 * each leaf holds at most eight, so every one of them is referenced once.
 *
 * Covers the base: bd5b1376 added the halving to the still build, and 165a2635 to the moving one.
 * Checked by making the halving in BinaryBuilder::node() drop the last reference, and by deleting
 * the halving in TemporalBuilder::node(), each of which fails this case.
 **/
BOOST_AUTO_TEST_CASE(moya_builder_coincident_centroids_are_halved_in_order_test) {
    CpuDevice device;
    std::vector<glm::vec3> points;
    for (int i = 0; i < 40; ++i) {
        points.insert(points.end(), { { 0.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 1.0F } });
    }
    const std::vector<accel::Box> bounds = accel::triangleBounds(points);
    std::vector<std::uint32_t> every(bounds.size());
    std::iota(every.begin(), every.end(), 0U);

    const accel::Hierarchy still = accel::build(device, bounds, accel::triangleClip(points), { .spatial = false });
    BOOST_TEST(sorted(still.references) == every, boost::test_tools::per_element());
    BOOST_TEST(fullestLeaf(still.nodes) <= 8U);

    const accel::Sweep sweep = [&bounds](std::uint32_t primitive, float, float) {
        return accel::LinearBox::still(bounds[primitive]);
    };
    const accel::MovingHierarchy moving = accel::buildMoving(device, static_cast<std::uint32_t>(bounds.size()), sweep, 0.0F, 1.0F, { .temporal = false });
    BOOST_TEST(sorted(moving.references) == every, boost::test_tools::per_element());
    BOOST_TEST(fullestLeaf(moving.nodes) <= 8U);
}

/**
 * A moving hierarchy is refused a time range that is not finite or ends before it begins, and a
 * count of primitives a leaf's first reference cannot name. A range that begins and ends at once
 * is built.
 *
 * Covers the base: 165a2635 added the checks to buildMoving(). Checked by deleting the range's
 * check and by letting the count's pass at 2^31, each of which fails this case.
 **/
BOOST_AUTO_TEST_CASE(moya_builder_a_moving_build_refuses_a_bad_range_or_count_test) {
    CpuDevice device;
    const accel::Sweep sweep = [](std::uint32_t, float, float) {
        return accel::LinearBox::still(accel::Box { .lower = glm::vec3 { 0.0F }, .upper = glm::vec3 { 1.0F } });
    };
    // a build that goes ahead with 2^31 primitives runs out of memory rather than refusing them
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the count, then the range
    const auto refused = [&](std::uint32_t count, float begin, float end) {
        try {
            static_cast<void>(accel::buildMoving(device, count, sweep, begin, end));
        } catch (const std::invalid_argument &) {
            return true;
        } catch (const std::exception &) {
            return false;
        }
        return false;
    };
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    const std::pair<float, float> ranges[] = { { nan, 1.0F }, { 0.0F, nan }, { -infinity, 1.0F }, { 0.0F, infinity }, { 1.0F, 0.0F } };
    for (const auto & [begin, end] : ranges) {
        BOOST_TEST(refused(1, begin, end), "from " << begin << " to " << end);
    }
    for (const std::uint32_t count : { cpu::kLeaf, std::numeric_limits<std::uint32_t>::max() }) {
        BOOST_TEST(refused(count, 0.0F, 1.0F), count << " primitives");
    }
    BOOST_TEST(accel::buildMoving(device, 1, sweep, 0.5F, 0.5F).references.size() == 1U);
}

};  // namespace
};  // namespace v3d::moya::tracing

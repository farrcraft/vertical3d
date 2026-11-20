/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/light/internal/Tree.h>

#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/light/Bounds.h>
#include <moya/libmoya/kernel/light/Tree.h>
#include <moya/libmoya/light/internal/Pending.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::light::internal {

namespace {

constexpr float kPi = 3.14159265358979323846F;

/** The planes a node's split is chosen among, on each axis. **/
constexpr std::size_t kBuckets = 12;

/** The angle whose cosine is given, held to [0, pi]. **/
float angleOf(float cosine) {
    return std::acos(std::clamp(cosine, -1.0F, 1.0F));
}

/** The box's surface area, zero for a box of no extent. **/
float area(const kernel::light::Bounds & bounds) {
    const glm::vec3 size = glm::max(bounds.upper - bounds.lower, glm::vec3 { 0.0F });
    return 2.0F * ((size.x * size.y) + (size.y * size.z) + (size.z * size.x));
}

/**
 * What bounds cost a split along an axis of a node's box, as Conty Estevez and Kulla measure it.
 * It is their power, times the solid angle their cone reaches weighed by the cosine of its
 * falloff, times their box's area. A node much shorter on the axis than on its longest costs
 * more, so long thin splits are preferred.
 **/
float cost(const kernel::light::Bounds & bounds, const glm::vec3 & extent, int axis) {
    const float spread = angleOf(bounds.cosSpread);
    const float reach = std::fmin(spread + angleOf(bounds.cosFalloff), kPi);
    const float sinSpread = std::sin(spread);
    const float solid = (2.0F * kPi * (1.0F - bounds.cosSpread)) +
        (0.5F * kPi * ((2.0F * reach * sinSpread) - std::cos(spread - (2.0F * reach)) - (2.0F * spread * sinSpread) + bounds.cosSpread));
    const float longest = std::fmax(extent.x, std::fmax(extent.y, extent.z));
    const float along = extent[axis] > 0.0F ? longest / extent[axis] : 1.0F;
    return bounds.power * solid * along * area(bounds);
}

glm::vec3 centreOf(const Leaf & leaf) {
    return 0.5F * (leaf.bounds.lower + leaf.bounds.upper);
}

/** The bucket a centre falls in along an axis of the box of centres. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the centre, then the box's corners
std::size_t bucketOf(const glm::vec3 & centre, const glm::vec3 & lower, const glm::vec3 & upper, int axis) {
    const float along = (centre[axis] - lower[axis]) / (upper[axis] - lower[axis]);
    const auto bucket = static_cast<std::size_t>(std::fmax(along, 0.0F) * static_cast<float>(kBuckets));  // checked: the centre lies in the box, so 0 to kBuckets
    return std::min(bucket, kBuckets - 1);
}

/**
 * Where a range of leaves splits, by the surface area and orientation heuristic, as the leaves are
 * reordered about it. The middle when no plane parts their centres.
 **/
std::size_t split(std::vector<Leaf> & leaves, std::size_t begin, std::size_t end, const kernel::light::Bounds & whole) {
    glm::vec3 lower { std::numeric_limits<float>::infinity() };
    glm::vec3 upper { -std::numeric_limits<float>::infinity() };
    for (std::size_t i = begin; i < end; ++i) {
        lower = glm::min(lower, centreOf(leaves[i]));
        upper = glm::max(upper, centreOf(leaves[i]));
    }
    const glm::vec3 extent = whole.upper - whole.lower;
    float least = std::numeric_limits<float>::infinity();
    int bestAxis = -1;
    std::size_t bestBucket = 0;
    for (int axis = 0; axis < 3; ++axis) {
        if (!(upper[axis] > lower[axis])) {
            continue;
        }
        std::array<kernel::light::Bounds, kBuckets> buckets {};
        for (std::size_t i = begin; i < end; ++i) {
            const std::size_t bucket = bucketOf(centreOf(leaves[i]), lower, upper, axis);
            buckets[bucket] = united(buckets[bucket], leaves[i].bounds);
        }
        for (std::size_t plane = 0; plane + 1 < kBuckets; ++plane) {
            kernel::light::Bounds below;
            kernel::light::Bounds above;
            for (std::size_t bucket = 0; bucket <= plane; ++bucket) {
                below = united(below, buckets[bucket]);
            }
            for (std::size_t bucket = plane + 1; bucket < kBuckets; ++bucket) {
                above = united(above, buckets[bucket]);
            }
            // a plane with every leaf on one side parts nothing
            if (!(below.power > 0.0F) || !(above.power > 0.0F)) {
                continue;
            }
            const float total = cost(below, extent, axis) + cost(above, extent, axis);
            if (total < least) {
                least = total;
                bestAxis = axis;
                bestBucket = plane;
            }
        }
    }
    const std::size_t middle = begin + ((end - begin) / 2);
    if (bestAxis < 0) {
        return middle;
    }
    const auto first = leaves.begin() + static_cast<std::ptrdiff_t>(begin);
    const auto last = leaves.begin() + static_cast<std::ptrdiff_t>(end);
    const auto parted = std::stable_partition(first, last, [&](const Leaf & leaf) {
        return bucketOf(centreOf(leaf), lower, upper, bestAxis) <= bestBucket;
    });
    const auto at = static_cast<std::size_t>(parted - leaves.begin());
    return at == begin || at == end ? middle : at;
}

};  // namespace

kernel::light::Bounds everyDirection(const glm::vec3 & lower, const glm::vec3 & upper) {
    kernel::light::Bounds out;
    out.lower = lower;
    out.upper = upper;
    out.cosSpread = -1.0F;
    out.cosFalloff = 0.0F;
    return out;
}

kernel::light::Bounds united(const kernel::light::Bounds & left, const kernel::light::Bounds & right) {
    if (!(left.power > 0.0F)) {
        return right;
    }
    if (!(right.power > 0.0F)) {
        return left;
    }
    kernel::light::Bounds out;
    out.lower = glm::min(left.lower, right.lower);
    out.upper = glm::max(left.upper, right.upper);
    out.power = left.power + right.power;
    out.cosFalloff = std::fmin(left.cosFalloff, right.cosFalloff);
    const float leftSpread = angleOf(left.cosSpread);
    const float rightSpread = angleOf(right.cosSpread);
    const float between = angleOf(glm::dot(left.axis, right.axis));
    // one cone may hold the other
    if (std::fmin(between + rightSpread, kPi) <= leftSpread) {
        out.axis = left.axis;
        out.cosSpread = left.cosSpread;
        return out;
    }
    if (std::fmin(between + leftSpread, kPi) <= rightSpread) {
        out.axis = right.axis;
        out.cosSpread = right.cosSpread;
        return out;
    }
    const float spread = 0.5F * (leftSpread + between + rightSpread);
    const glm::vec3 turn = glm::cross(left.axis, right.axis);
    out.axis = left.axis;
    out.cosSpread = -1.0F;
    if (spread >= kPi || !(glm::dot(turn, turn) > 0.0F)) {
        return out;
    }
    // the axis turns from the left's towards the right's, until the cone just holds the left's
    out.axis = glm::normalize(glm::angleAxis(spread - leftSpread, glm::normalize(turn)) * left.axis);
    out.cosSpread = std::cos(spread);
    return out;
}

std::uint32_t build(std::vector<Leaf> leaves, std::vector<kernel::light::TreeNode> & nodes, std::vector<std::uint32_t> & leafOf, std::uint32_t first) {
    leaves.erase(std::remove_if(leaves.begin(), leaves.end(), [](const Leaf & leaf) { return !(leaf.bounds.power > 0.0F); }), leaves.end());
    if (leaves.empty()) {
        return kernel::geometry::kNoPrimitive;
    }
    const auto root = static_cast<std::uint32_t>(nodes.size());
    std::vector<Pending> pending { Pending { .begin = 0, .end = leaves.size() } };
    while (!pending.empty()) {
        const Pending range = pending.back();
        pending.pop_back();
        const auto at = static_cast<std::uint32_t>(nodes.size());
        nodes.emplace_back();
        nodes[at].parent = range.parent;
        if (range.parent != kernel::geometry::kNoPrimitive) {
            (range.left ? nodes[range.parent].left : nodes[range.parent].right) = at;
        }
        if (range.end - range.begin == 1) {
            const Leaf & leaf = leaves[range.begin];
            nodes[at].bounds = leaf.bounds;
            nodes[at].left = leaf.held;
            leafOf[first + leaf.held] = at;
            continue;
        }
        kernel::light::Bounds whole;
        for (std::size_t i = range.begin; i < range.end; ++i) {
            whole = united(whole, leaves[i].bounds);
        }
        nodes[at].bounds = whole;
        const std::size_t middle = split(leaves, range.begin, range.end, whole);
        // the left is built first, so it is taken from the back last
        pending.push_back(Pending { .begin = middle, .end = range.end, .parent = at, .left = false });
        pending.push_back(Pending { .begin = range.begin, .end = middle, .parent = at, .left = true });
    }
    return root;
}

};  // namespace v3d::moya::light::internal

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/light/Bounds.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/sample/Sobol.h>

#include <cmath>
#include <cstdint>
#include <limits>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::light {

/**
 * A node of the light tree. An inner node has two children. A leaf has none, and left holds what
 * it bounds: a light's place among the lights in the top tree, or a triangle in a mesh light's
 * subtree. parent is kNoPrimitive at a tree's root.
 **/
struct TreeNode {
    Bounds bounds;
    std::uint32_t left { geometry::kNoPrimitive };
    std::uint32_t right { geometry::kNoPrimitive };
    std::uint32_t parent { geometry::kNoPrimitive };

    MOYA_KERNEL bool leaf() const {
        return right == geometry::kNoPrimitive;
    }
};

/**
 * The cosine of one angle less another, held to zero or more: one when the first is the smaller.
 * Each angle is given by its sine and cosine.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the angle, then what is taken off it
MOYA_KERNEL inline float cosLess(float sinA, float cosA, float sinB, float cosB) {
    return cosA > cosB ? 1.0F : (cosA * cosB) + (sinA * sinB);
}

/** The sine of one angle less another, held to zero or more, as cosLess() gives its cosine. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the angle, then what is taken off it
MOYA_KERNEL inline float sinLess(float sinA, float cosA, float sinB, float cosB) {
    return cosA > cosB ? 0.0F : (sinA * cosB) - (cosA * sinB);
}

/** The sine of an angle from its cosine. **/
MOYA_KERNEL inline float sineOf(float cosine) {
    return std::sqrt(std::fmax(0.0F, 1.0F - (cosine * cosine)));
}

/**
 * A bound on what a node's lights bring to a point, which a walk draws a child by. It is the
 * power over the distance squared to the box's centre, times a cosine. That cosine's angle is the
 * least between the direction to the point and the cone, less the angle the box subtends from the
 * point. It is zero where that angle reaches the falloff, as no light of the node emits towards
 * the point. With a normal, it is also times the cosine of the least angle between the normal,
 * either way, and a direction to the box.
 *
 * The distance is held to at least the box's half diagonal. From inside the sphere about the box,
 * the box subtends every direction. The bound is held to half the largest float, so the sum of
 * two children's is finite.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, then its normal
MOYA_KERNEL inline float importance(const Bounds & bounds, const glm::vec3 & point, const glm::vec3 & normal) {
    if (!(bounds.power > 0.0F)) {
        return 0.0F;
    }
    const glm::vec3 centre = 0.5F * (bounds.lower + bounds.upper);
    const glm::vec3 half = 0.5F * (bounds.upper - bounds.lower);
    const float radius2 = glm::dot(half, half);
    const glm::vec3 between = point - centre;
    const float distance2 = glm::dot(between, between);
    // the directions from the point to the box's sphere
    float cosBox = -1.0F;
    float sinBox = 0.0F;
    if (distance2 > radius2) {
        const float sin2 = radius2 / distance2;
        cosBox = std::sqrt(1.0F - sin2);
        sinBox = std::sqrt(sin2);
    }
    const glm::vec3 away = distance2 > 0.0F ? between / std::sqrt(distance2) : bounds.axis;
    const float cosOff = glm::dot(bounds.axis, away);
    const float sinOff = sineOf(cosOff);
    const float sinSpread = sineOf(bounds.cosSpread);
    const float cosOutside = cosLess(sinOff, cosOff, sinSpread, bounds.cosSpread);
    const float sinOutside = sinLess(sinOff, cosOff, sinSpread, bounds.cosSpread);
    const float cosNearest = cosLess(sinOutside, cosOutside, sinBox, cosBox);
    if (cosNearest < bounds.cosFalloff) {
        return 0.0F;
    }
    float out = bounds.power * cosNearest / std::fmax(std::fmax(distance2, radius2), std::numeric_limits<float>::min());
    if (glm::dot(normal, normal) > 0.0F) {
        const float cosNormal = std::fabs(glm::dot(away, normal));
        out *= cosLess(sineOf(cosNormal), cosNormal, sinBox, cosBox);
    }
    return std::fmin(std::fmax(out, 0.0F), 0.5F * std::numeric_limits<float>::max());
}

/**
 * The chance that a walk from an inner node takes its left child at a point: its importance over
 * the sum of both children's. Below zero when neither brings anything.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, then its normal
MOYA_KERNEL inline float leftChance(const Array<TreeNode> & nodes, const TreeNode & inner, const glm::vec3 & point, const glm::vec3 & normal) {
    const float left = importance(nodes[inner.left].bounds, point, normal);
    const float sum = left + importance(nodes[inner.right].bounds, point, normal);
    return sum > 0.0F ? left / sum : -1.0F;
}

/**
 * A leaf drawn from a tree's root with one random number, each child in proportion to its
 * importance at a point. The random number is rescaled within each branch taken, so a walk on into
 * another tree draws by what is left of it. The chance of the leaf multiplies chance. kNoPrimitive
 * when a node's children bring nothing to the point.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, then its normal
MOYA_KERNEL inline std::uint32_t walk(const Array<TreeNode> & nodes, std::uint32_t root, const glm::vec3 & point, const glm::vec3 & normal, float & u,
    float & chance) {
    std::uint32_t at = root;
    while (!nodes[at].leaf()) {
        const TreeNode & inner = nodes[at];
        const float left = leftChance(nodes, inner, point, normal);
        if (left < 0.0F) {
            return geometry::kNoPrimitive;
        }
        if (u < left) {
            u = std::fmin(u / left, sample::kOneMinusEpsilon);
            chance *= left;
            at = inner.left;
        } else {
            u = std::fmin((u - left) / (1.0F - left), sample::kOneMinusEpsilon);
            chance *= 1.0F - left;
            at = inner.right;
        }
    }
    return at;
}

/**
 * The chance that walk() reaches a leaf from its tree's root at a point, found from the leaf up.
 * Each step's chance is computed as walk() computes it, so the two agree to rounding.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, then its normal
MOYA_KERNEL inline float reached(const Array<TreeNode> & nodes, std::uint32_t leaf, const glm::vec3 & point, const glm::vec3 & normal) {
    float chance = 1.0F;
    std::uint32_t at = leaf;
    while (nodes[at].parent != geometry::kNoPrimitive) {
        const TreeNode & inner = nodes[nodes[at].parent];
        const float left = leftChance(nodes, inner, point, normal);
        if (left < 0.0F) {
            return 0.0F;
        }
        chance *= inner.left == at ? left : 1.0F - left;
        at = nodes[at].parent;
    }
    return chance;
}

};  // namespace v3d::moya::kernel::light

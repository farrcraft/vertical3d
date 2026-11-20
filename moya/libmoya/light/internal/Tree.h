/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/light/Bounds.h>
#include <moya/libmoya/kernel/light/Tree.h>

#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::moya::light::internal {

/** What a tree is built over: one light, or one triangle of a mesh light, and its bounds. **/
struct Leaf {
    kernel::light::Bounds bounds;

    /** What the leaf holds: a light's place among the lights, or a triangle of a geometry. **/
    std::uint32_t held { 0 };
};

/** Bounds of a box and of every direction, with no power. **/
kernel::light::Bounds everyDirection(const glm::vec3 & lower, const glm::vec3 & upper);

/**
 * The bounds of everything two bounds hold: their box, the smallest cone about an axis between
 * their axes that holds both cones, the wider falloff, and their power. Bounds of no power add
 * nothing.
 **/
kernel::light::Bounds united(const kernel::light::Bounds & left, const kernel::light::Bounds & right);

/**
 * A tree over leaves, appended to nodes, and the place of its root among them. Each leaf with
 * power has a node, whose place is written to leafOf at first plus what it holds. A leaf with no
 * power has none. kNoPrimitive when no leaf has any power.
 *
 * A node splits its leaves by the surface area and orientation heuristic. Twelve planes cross the
 * box of the leaves' centres on each axis, and it splits at the one whose two sides cost least. A
 * side costs its power, times the solid angle its cone reaches, times its box's area. Where no
 * plane parts the centres, it splits them in half in the order given. The tree depends on the
 * leaves and their order alone.
 **/
std::uint32_t build(std::vector<Leaf> leaves, std::vector<kernel::light::TreeNode> & nodes, std::vector<std::uint32_t> & leafOf, std::uint32_t first);

};  // namespace v3d::moya::light::internal

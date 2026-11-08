/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/cpu/Instance.h>
#include <moya/libmoya/device/cpu/Node.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::device::cpu {

/**
 * The CPU's traversal: a hierarchy of eight-wide nodes for each mesh and each group, whose nodes
 * are tested eight children at a time with SIMD. Every hierarchy is held in the one array of
 * nodes, and a ray starts at the world's.
 *
 * A ray enters an instance only when the instance is visible to its kind and the chain it is
 * inside has room for one more. A hit's chain is the instances it entered, outermost first.
 *
 * Only the CPU's libraries define nearest() and any(), each for its own instruction set. libmoya
 * fills the arrays and passes the traversal to a library's entry point, and never calls either.
 **/
struct Traversal {
    kernel::Array<Node> nodes;

    /**
     * The hierarchies' references. A mesh's leaf holds the index of a triangle within its mesh,
     * and a group's the index of an instance.
     **/
    kernel::Array<std::uint32_t> references;

    /** Three points per triangle of every mesh, each in its mesh's own space. **/
    kernel::Array<glm::vec3> points;

    kernel::Array<Instance> instances;

    /** The root of the world's hierarchy, whose leaves hold the instances in no group. **/
    std::uint32_t world { 0 };

    kernel::geometry::Hit nearest(const kernel::geometry::Ray & ray) const;
    bool any(const kernel::geometry::Ray & ray) const;
};

};  // namespace v3d::moya::device::cpu

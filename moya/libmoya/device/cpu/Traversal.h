/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/cpu/Node.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/BruteForce.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>

#include <cstdint>

namespace v3d::moya::device::cpu {

/**
 * The CPU's traversal: a hierarchy of eight-wide nodes over the placed triangles, whose nodes are
 * tested eight children at a time with SIMD.
 *
 * Only the CPU's libraries define nearest() and any(), each for its own instruction set. libmoya
 * fills the arrays and passes the traversal to a library's entry point, and never calls either.
 **/
struct Traversal {
    kernel::Array<Node> nodes;

    /** The hierarchy's references, each a placed triangle's index. **/
    kernel::Array<std::uint32_t> references;

    kernel::geometry::Placed placed;

    /** The placement each placed triangle belongs to, one per triangle. **/
    kernel::Array<std::uint32_t> runs;

    kernel::geometry::Hit nearest(const kernel::geometry::Ray & ray) const;
    bool any(const kernel::geometry::Ray & ray) const;
};

};  // namespace v3d::moya::device::cpu

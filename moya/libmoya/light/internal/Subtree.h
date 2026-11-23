/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/light/Bounds.h>
#include <moya/libmoya/kernel/light/Tree.h>

#include <cstdint>

namespace v3d::moya::light::internal {

/**
 * A subtree over a geometry's triangles: its root among the nodes, or kNoPrimitive when no
 * triangle has any area, the first of its triangles' leaves, and its root's bounds.
 **/
struct Subtree {
    std::uint32_t root { kernel::geometry::kNoPrimitive };
    std::uint32_t leaves { 0 };
    kernel::light::Bounds bounds;
};

};  // namespace v3d::moya::light::internal

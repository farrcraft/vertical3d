/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>

#include <cstddef>
#include <cstdint>

namespace v3d::moya::light::internal {

/**
 * A range of leaves that build() has still to make a node for, and the node it is a child of:
 * its left child, or its right. A root's parent is kNoPrimitive.
 **/
struct Pending {
    std::size_t begin { 0 };
    std::size_t end { 0 };
    std::uint32_t parent { kernel::geometry::kNoPrimitive };
    bool left { true };
};

};  // namespace v3d::moya::light::internal

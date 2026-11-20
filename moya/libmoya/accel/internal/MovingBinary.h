/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/internal/MovingBinaryNode.h>

#include <cstdint>
#include <vector>

namespace v3d::moya::accel::internal {

/** A binary hierarchy over moving primitives, or part of one: its root is its first node. **/
struct MovingBinary {
    std::vector<MovingBinaryNode> nodes;
    std::vector<std::uint32_t> references;
};

};  // namespace v3d::moya::accel::internal

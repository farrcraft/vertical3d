/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/internal/BinaryNode.h>

#include <cstdint>
#include <vector>

namespace v3d::moya::accel::internal {

/** A binary hierarchy, or part of one: its root is its first node. **/
struct Binary {
    std::vector<BinaryNode> nodes;
    std::vector<std::uint32_t> references;
};

};  // namespace v3d::moya::accel::internal

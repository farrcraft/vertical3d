/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::accel {

/** Where a prototype's hierarchy starts, and whether its nodes are moving ones. **/
struct Root {
    std::uint32_t node { 0 };
    bool moving { false };
};

};  // namespace v3d::moya::accel

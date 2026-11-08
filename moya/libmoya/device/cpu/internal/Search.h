/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/cpu/Traversal.h>
#include <moya/libmoya/kernel/geometry/Hit.h>

#include <cstdint>

namespace v3d::moya::device::cpu::internal {

/** The chain of instances a ray is inside, outermost first. **/
struct Chain {
    std::uint32_t instances[kernel::geometry::kInstanceDepth] {};
    std::uint32_t depth { 0 };
};

/** What a search for the nearest hit keeps as it enters instances and leaves them. **/
struct Nearest {
    const Traversal & traversal;
    Chain chain;
    float closest { 0.0F };
    kernel::geometry::Hit hit;
};

};  // namespace v3d::moya::device::cpu::internal

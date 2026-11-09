/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Sweep.h>

#include <cstdint>

namespace v3d::moya::accel::internal {

/** A primitive as the builder sorts it, with its bounds over the time range of its node. **/
struct MovingReference {
    LinearBox box;
    std::uint32_t primitive { 0 };
};

};  // namespace v3d::moya::accel::internal

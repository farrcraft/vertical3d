/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Sweep.h>

#include <cstdint>

namespace v3d::moya::accel::internal {

/** A node of the binary hierarchy before it is collapsed, over a time range. **/
struct MovingBinaryNode {
    LinearBox box;
    float begin { 0.0F };
    float end { 0.0F };
    bool leaf { false };
    std::uint32_t left { 0 };
    std::uint32_t right { 0 };
    std::uint32_t first { 0 };
    std::uint32_t count { 0 };
};

};  // namespace v3d::moya::accel::internal

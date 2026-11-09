/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/device/cpu/MotionNode.h>

#include <cstdint>
#include <vector>

namespace v3d::moya::accel {

/**
 * A built hierarchy over primitives that move, in host memory, over the time range from begin to
 * end. Its nodes are as Hierarchy's, with each child's box at the start and end of the child's
 * own range.
 **/
struct MovingHierarchy {
    std::vector<device::cpu::MotionNode> nodes;
    std::vector<std::uint32_t> references;

    /** The bounds of everything the hierarchy holds over its range. **/
    LinearBox bounds;
    float begin { 0.0F };
    float end { 0.0F };
};

};  // namespace v3d::moya::accel

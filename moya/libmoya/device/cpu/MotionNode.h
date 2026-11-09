/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/cpu/Node.h>

#include <cstdint>

namespace v3d::moya::device::cpu {

/**
 * A node of a hierarchy over geometry that moves. Each child holds a box at the start and at the
 * end of the time range it covers, and the box at a time between is interpolated linearly. A ray
 * outside a child's range does not reach it.
 *
 * The planes at the start are stored as a Node stores its planes, and those at the end after
 * them. A child is linked as in a Node.
 **/
struct alignas(32) MotionNode {
    float lowerX[kWidth];
    float upperX[kWidth];
    float lowerY[kWidth];
    float upperY[kWidth];
    float lowerZ[kWidth];
    float upperZ[kWidth];
    float endLowerX[kWidth];
    float endUpperX[kWidth];
    float endLowerY[kWidth];
    float endUpperY[kWidth];
    float endLowerZ[kWidth];
    float endUpperZ[kWidth];
    float begin[kWidth];
    float end[kWidth];
    std::uint32_t child[kWidth];
    std::uint32_t count[kWidth];
};

};  // namespace v3d::moya::device::cpu

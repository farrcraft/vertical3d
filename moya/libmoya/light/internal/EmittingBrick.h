/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/light/Brick.h>

#include <vector>

namespace v3d::moya::light::internal {

/** A brick of a grid that emits, with the weight of each of its cells and their sum. **/
struct EmittingBrick {
    kernel::light::Brick brick;
    std::vector<float> weights;
    float sum { 0.0F };
};

};  // namespace v3d::moya::light::internal

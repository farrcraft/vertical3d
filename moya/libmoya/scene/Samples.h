/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/scene/Value.h>

#include <vector>

namespace v3d::moya::scene {

/**
 * A parameter's value over the shutter. With no times, its one value holds at every time.
 * Otherwise each value holds at the time beside it, the times rise strictly, and every value has
 * the same type and size.
 **/
struct Samples {
    std::vector<float> times;
    std::vector<Value> values;

    bool operator==(const Samples & other) const = default;
};

};  // namespace v3d::moya::scene

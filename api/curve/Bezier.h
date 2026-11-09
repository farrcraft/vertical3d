/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <vector>

#include "BezierPoint.h"

namespace v3d::curve {

/**
 * A curve of cubic segments between points, each shaped by the handles at its two ends.
 **/
struct Bezier final {
    std::vector<BezierPoint> points;

    bool operator==(const Bezier& other) const = default;
};

};  // namespace v3d::curve

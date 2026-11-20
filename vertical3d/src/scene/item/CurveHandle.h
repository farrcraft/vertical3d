/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/curve/Curve.h>

#include "CurvePoint.h"

namespace v3d::editor::scene::item {

/**
 * One handle of a Bezier point among the curves of an item.
 **/
struct CurveHandle final {
    CurvePoint at;
    v3d::curve::Side side = v3d::curve::Side::Left;

    bool operator==(const CurveHandle& other) const = default;
};

};  // namespace v3d::editor::scene::item

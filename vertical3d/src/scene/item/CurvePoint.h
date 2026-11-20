/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>

namespace v3d::editor::scene::item {

/**
 * One control point among the curves of an item: which curve, and which of its points.
 *
 * A curve item's parts are numbered across every curve it holds, in order. Its control points
 * are numbered from zero, the first curve's first, so a point's number is the count of points
 * in the curves before it plus its place in its own curve. Every curve type has control points:
 * a poly, Catmull-Rom or NURBS curve's points, and a Bezier curve's points without their
 * handles. Each control point has two handle numbers, twice its own number for its left handle
 * and one more for its right, and only those of a Bezier point name a handle.
 *
 * A number names whatever part holds it now, so adding or removing a curve renumbers the parts
 * of every curve after it.
 **/
struct CurvePoint final {
    std::size_t curve = 0;
    std::size_t point = 0;

    bool operator==(const CurvePoint& other) const = default;
};

};  // namespace v3d::editor::scene::item

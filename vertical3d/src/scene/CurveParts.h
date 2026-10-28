/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/Index.h>
#include <api/curve/Curve.h>
#include <api/curve/Curves.h>

#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::editor {

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

/**
 * One handle of a Bezier point among the curves of an item.
 **/
struct CurveHandle final {
    CurvePoint at;
    v3d::curve::Side side = v3d::curve::Side::Left;

    bool operator==(const CurveHandle& other) const = default;
};

/**
 * @return how many control points a curve has
 **/
std::size_t controlCount(const v3d::curve::Curve& curve);

/**
 * @param point below controlCount(curve)
 * @return where the control point is, in the item's space
 **/
glm::vec3 controlPoint(const v3d::curve::Curve& curve, std::size_t point);

/**
 * Move a control point through a function. A Bezier point's handles move through the same
 * function, so a turn about a pivot turns them with the point.
 *
 * @param point below controlCount(*curve)
 **/
void moveControl(v3d::curve::Curve* curve, std::size_t point, const std::function<glm::vec3(const glm::vec3&)>& place);

/**
 * @return how many control points every curve of the geometry has together
 **/
std::size_t pointCount(const v3d::curve::Curves& curves);

/**
 * @return the control point a part number names, or nothing when the geometry has no such point
 **/
std::optional<CurvePoint> pointAt(const v3d::curve::Curves& curves, v3d::brep::Index part);

/**
 * @param at a control point the geometry has
 * @return the part number of the control point
 **/
v3d::brep::Index pointPart(const v3d::curve::Curves& curves, const CurvePoint& at);

/**
 * @return the handle a part number names, or nothing when the geometry has no such point or
 *         its curve is not a Bezier curve
 **/
std::optional<CurveHandle> handleAt(const v3d::curve::Curves& curves, v3d::brep::Index part);

/**
 * @param handle a handle of a Bezier point the geometry has
 * @return the part number of the handle
 **/
v3d::brep::Index handlePart(const v3d::curve::Curves& curves, const CurveHandle& handle);

/**
 * @param handle a handle of a Bezier point the geometry has
 * @return where the handle is, in the item's space
 **/
glm::vec3 handlePosition(const v3d::curve::Curves& curves, const CurveHandle& handle);

/**
 * Every control point of the geometry with its part number, in part order.
 **/
std::vector<std::pair<v3d::brep::Index, glm::vec3>> controlPoints(const v3d::curve::Curves& curves);

/**
 * Every handle of every Bezier point of the geometry with its part number, in part order.
 **/
std::vector<std::pair<v3d::brep::Index, glm::vec3>> handlePoints(const v3d::curve::Curves& curves);

};  // namespace v3d::editor

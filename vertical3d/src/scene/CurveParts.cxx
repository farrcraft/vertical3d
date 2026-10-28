/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CurveParts.h"

#include <api/brep/Index.h>
#include <api/curve/Curve.h>
#include <api/curve/Curves.h>

#include <cstddef>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::editor {

namespace {

/**
 * Two handle numbers for every control point, the left one first.
 **/
constexpr v3d::brep::Index HANDLES = 2;

};  // namespace

/**
 **/
std::size_t controlCount(const v3d::curve::Curve& curve) {
    return std::visit([](const auto& shape) { return shape.points.size(); }, curve.shape);
}

/**
 **/
glm::vec3 controlPoint(const v3d::curve::Curve& curve, std::size_t point) {
    return std::visit(
        [point](const auto& shape) {
            using Shape = std::decay_t<decltype(shape)>;
            if constexpr (std::is_same_v<Shape, v3d::curve::Bezier>) {
                return shape.points[point].position;
            } else {
                return shape.points[point];
            }
        },
        curve.shape);
}

/**
 **/
void moveControl(v3d::curve::Curve* curve, std::size_t point, const std::function<glm::vec3(const glm::vec3&)>& place) {
    std::visit(
        [point, &place](auto& shape) {
            using Shape = std::decay_t<decltype(shape)>;
            if constexpr (std::is_same_v<Shape, v3d::curve::Bezier>) {
                v3d::curve::BezierPoint& moved = shape.points[point];
                moved.position = place(moved.position);
                moved.left = place(moved.left);
                moved.right = place(moved.right);
            } else {
                shape.points[point] = place(shape.points[point]);
            }
        },
        curve->shape);
}

/**
 **/
std::size_t pointCount(const v3d::curve::Curves& curves) {
    std::size_t count = 0;
    for (std::size_t index = 0; index < curves.size(); index++) {
        count += controlCount(curves.curve(index));
    }
    return count;
}

/**
 **/
std::optional<CurvePoint> pointAt(const v3d::curve::Curves& curves, v3d::brep::Index part) {
    std::size_t rest = part;
    for (std::size_t index = 0; index < curves.size(); index++) {
        const std::size_t count = controlCount(curves.curve(index));
        if (rest < count) {
            return CurvePoint{ .curve = index, .point = rest };
        }
        rest -= count;
    }
    return std::nullopt;
}

/**
 **/
v3d::brep::Index pointPart(const v3d::curve::Curves& curves, const CurvePoint& at) {
    std::size_t part = at.point;
    for (std::size_t index = 0; index < at.curve; index++) {
        part += controlCount(curves.curve(index));
    }
    return static_cast<v3d::brep::Index>(part);
}

/**
 **/
std::optional<CurveHandle> handleAt(const v3d::curve::Curves& curves, v3d::brep::Index part) {
    const std::optional<CurvePoint> at = pointAt(curves, part / HANDLES);
    if (!at || !std::holds_alternative<v3d::curve::Bezier>(curves.curve(at->curve).shape)) {
        return std::nullopt;
    }
    return CurveHandle{ .at = *at, .side = (part % HANDLES) == 0 ? v3d::curve::Side::Left : v3d::curve::Side::Right };
}

/**
 **/
v3d::brep::Index handlePart(const v3d::curve::Curves& curves, const CurveHandle& handle) {
    return (pointPart(curves, handle.at) * HANDLES) + (handle.side == v3d::curve::Side::Left ? 0 : 1);
}

/**
 **/
glm::vec3 handlePosition(const v3d::curve::Curves& curves, const CurveHandle& handle) {
    const v3d::curve::BezierPoint& point = std::get<v3d::curve::Bezier>(curves.curve(handle.at.curve).shape).points[handle.at.point];
    return handle.side == v3d::curve::Side::Left ? point.left : point.right;
}

/**
 **/
std::vector<std::pair<v3d::brep::Index, glm::vec3>> controlPoints(const v3d::curve::Curves& curves) {
    std::vector<std::pair<v3d::brep::Index, glm::vec3>> points;
    v3d::brep::Index part = 0;
    for (std::size_t index = 0; index < curves.size(); index++) {
        const v3d::curve::Curve& curve = curves.curve(index);
        const std::size_t count = controlCount(curve);
        for (std::size_t point = 0; point < count; point++) {
            points.emplace_back(part++, controlPoint(curve, point));
        }
    }
    return points;
}

/**
 **/
std::vector<std::pair<v3d::brep::Index, glm::vec3>> handlePoints(const v3d::curve::Curves& curves) {
    std::vector<std::pair<v3d::brep::Index, glm::vec3>> handles;
    v3d::brep::Index part = 0;
    for (std::size_t index = 0; index < curves.size(); index++) {
        const v3d::curve::Curve& curve = curves.curve(index);
        const auto* bezier = std::get_if<v3d::curve::Bezier>(&curve.shape);
        if (bezier == nullptr) {
            part += static_cast<v3d::brep::Index>(controlCount(curve));
            continue;
        }
        for (const v3d::curve::BezierPoint& point : bezier->points) {
            handles.emplace_back(part * HANDLES, point.left);
            handles.emplace_back((part * HANDLES) + 1, point.right);
            part++;
        }
    }
    return handles;
}

};  // namespace v3d::editor

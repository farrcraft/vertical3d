/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Curve.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "Segment.h"
#include "Spline.h"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::curve {

namespace {

constexpr float THIRD = 1.0F / 3.0F;

bool finite(const glm::vec3& point) {
    return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
}

bool finite(const std::vector<glm::vec3>& points) {
    return std::ranges::all_of(points, [](const glm::vec3& point) { return finite(point); });
}

std::string problem(const Poly& poly) {
    if (poly.points.size() < 2) {
        return "a poly curve needs two points";
    }
    return finite(poly.points) ? std::string() : "a point is not finite";
}

std::string problem(const CatmullRom& catmullRom) {
    if (catmullRom.points.size() < 2) {
        return "a Catmull-Rom curve needs two points";
    }
    return finite(catmullRom.points) ? std::string() : "a point is not finite";
}

std::string problem(const Bezier& bezier) {
    if (bezier.points.size() < 2) {
        return "a Bezier curve needs two points";
    }
    for (const BezierPoint& point : bezier.points) {
        if (!finite(point.position) || !finite(point.left) || !finite(point.right)) {
            return "a point or a handle is not finite";
        }
    }
    return {};
}

std::string problem(const Nurbs& nurbs, bool closed) {
    if (nurbs.order < 2) {
        return "a NURBS curve's order is at least 2";
    }
    if (nurbs.points.size() < nurbs.order) {
        return "a NURBS curve needs as many points as its order";
    }
    if (!finite(nurbs.points)) {
        return "a point is not finite";
    }
    if (!nurbs.weights.empty()) {
        if (nurbs.weights.size() != nurbs.points.size()) {
            return "a NURBS curve needs one weight per point";
        }
        for (const float weight : nurbs.weights) {
            if (!std::isfinite(weight) || weight <= 0.0F) {
                return "a weight is not positive";
            }
        }
    }
    if (nurbs.knots.empty()) {
        return {};
    }
    if (closed) {
        return "a closed NURBS curve has uniform knots";
    }
    if (nurbs.knots.size() != nurbs.points.size() + nurbs.order) {
        return "a NURBS curve needs as many knots as its points and its order";
    }
    for (std::size_t each = 0; each < nurbs.knots.size(); each++) {
        if (!std::isfinite(nurbs.knots[each]) || (each > 0 && nurbs.knots[each] < nurbs.knots[each - 1])) {
            return "the knots fall";
        }
    }
    const unsigned int degree = nurbs.order - 1;
    if (!(nurbs.knots[degree] < nurbs.knots[nurbs.points.size()])) {
        return "the knots leave no domain";
    }
    return {};
}

float step(unsigned int piece, unsigned int resolution) {
    return static_cast<float>(piece) / static_cast<float>(resolution);
}

std::vector<glm::vec3> evaluate(const Poly& poly, bool closed) {
    std::vector<glm::vec3> points = poly.points;
    if (closed) {
        points.push_back(points.front());
    }
    return points;
}

std::vector<glm::vec3> evaluate(const Bezier& bezier, bool closed, unsigned int resolution) {
    const std::size_t count = bezier.points.size();
    const std::size_t segments = closed ? count : count - 1;
    std::vector<glm::vec3> points{ bezier.points.front().position };
    for (std::size_t segment = 0; segment < segments; segment++) {
        const BezierPoint& from = bezier.points[segment];
        const BezierPoint& to = bezier.points[(segment + 1) % count];
        for (unsigned int piece = 1; piece <= resolution; piece++) {
            points.push_back(bezierAt(from.position, from.right, to.left, to.position, step(piece, resolution)));
        }
    }
    return points;
}

std::vector<glm::vec3> evaluate(const CatmullRom& curve, bool closed, unsigned int resolution) {
    const std::vector<glm::vec3>& from = curve.points;
    const std::size_t count = from.size();
    const std::size_t segments = closed ? count : count - 1;
    // an open curve's end points are their own neighbours beyond the ends
    auto at = [&](std::ptrdiff_t index) {
        const auto size = static_cast<std::ptrdiff_t>(count);
        if (closed) {
            return from[static_cast<std::size_t>(((index % size) + size) % size)];
        }
        return from[static_cast<std::size_t>(std::clamp<std::ptrdiff_t>(index, 0, size - 1))];
    };
    std::vector<glm::vec3> points{ from.front() };
    for (std::size_t segment = 0; segment < segments; segment++) {
        const auto index = static_cast<std::ptrdiff_t>(segment);
        for (unsigned int piece = 1; piece <= resolution; piece++) {
            points.push_back(catmullRomAt(at(index - 1), at(index), at(index + 1), at(index + 2), step(piece, resolution)));
        }
    }
    return points;
}

std::vector<glm::vec3> evaluate(const Nurbs& nurbs, bool closed, unsigned int resolution) {
    const Spline spline(nurbs, closed);
    const std::vector<std::size_t> spans = spline.spans();
    std::vector<glm::vec3> points{ spline.point(spans.front(), spline.knot(spans.front())) };
    for (const std::size_t span : spans) {
        const float start = spline.knot(span);
        const float length = spline.knot(span + 1) - start;
        for (unsigned int piece = 1; piece <= resolution; piece++) {
            points.push_back(spline.point(span, start + (length * step(piece, resolution))));
        }
    }
    return points;
}

/**
 * The points either side of a handle's point: toward is the one on the handle's side, and
 * across the one on the other side. Either is null at an open curve's end.
 **/
struct Neighbours {
    const glm::vec3* toward = nullptr;
    const glm::vec3* across = nullptr;
};

/**
 * Place a handle by its type. A handle with no neighbour on its side, and a Free or Aligned
 * one, is left where it is.
 **/
void place(Handle type, const glm::vec3& position, Neighbours neighbours, glm::vec3* handle) {
    if (neighbours.toward == nullptr || (type != Handle::Vector && type != Handle::Automatic)) {
        return;
    }
    const glm::vec3 near = *neighbours.toward - position;
    *handle = position + (near * THIRD);
    if (type == Handle::Vector || neighbours.across == nullptr) {
        return;
    }
    const glm::vec3 far = position - *neighbours.across;
    const float nearLength = glm::length(near);
    const float farLength = glm::length(far);
    if (nearLength <= 0.0F || farLength <= 0.0F) {
        return;
    }
    const glm::vec3 through = (near / nearLength) + (far / farLength);
    const float throughLength = glm::length(through);
    if (throughLength <= 0.0F) {
        return;
    }
    *handle = position + (through * (nearLength * THIRD / throughLength));
}

};  // namespace

std::string problem(const Curve& curve) {
    return std::visit(
        [&](const auto& shape) {
            using Shape = std::decay_t<decltype(shape)>;
            if constexpr (std::is_same_v<Shape, Nurbs>) {
                return problem(shape, curve.closed);
            } else {
                return problem(shape);
            }
        },
        curve.shape);
}

std::vector<glm::vec3> evaluate(const Curve& curve, unsigned int resolution) {
    return std::visit(
        [&](const auto& shape) {
            using Shape = std::decay_t<decltype(shape)>;
            if constexpr (std::is_same_v<Shape, Poly>) {
                return evaluate(shape, curve.closed);
            } else {
                return evaluate(shape, curve.closed, resolution);
            }
        },
        curve.shape);
}

void settle(Bezier* bezier, bool closed) {
    std::vector<BezierPoint>& points = bezier->points;
    const std::size_t count = points.size();
    if (count < 2) {
        return;
    }
    for (std::size_t each = 0; each < count; each++) {
        BezierPoint& point = points[each];
        const glm::vec3* before = (closed || each > 0) ? &points[(each + count - 1) % count].position : nullptr;
        const glm::vec3* after = (closed || each + 1 < count) ? &points[(each + 1) % count].position : nullptr;
        place(point.leftType, point.position, { .toward = before, .across = after }, &point.left);
        place(point.rightType, point.position, { .toward = after, .across = before }, &point.right);
    }
    if (closed) {
        return;
    }
    // an open curve's outer handles shape nothing, and mirror the inner ones
    BezierPoint& first = points.front();
    if (first.leftType == Handle::Vector || first.leftType == Handle::Automatic) {
        first.left = (2.0F * first.position) - first.right;
    }
    BezierPoint& last = points.back();
    if (last.rightType == Handle::Vector || last.rightType == Handle::Automatic) {
        last.right = (2.0F * last.position) - last.left;
    }
}

void moveHandle(Bezier* bezier, std::size_t point, Side side, const glm::vec3& position) {
    BezierPoint& moved = bezier->points[point];
    const bool left = side == Side::Left;
    Handle& type = left ? moved.leftType : moved.rightType;
    Handle& otherType = left ? moved.rightType : moved.leftType;
    glm::vec3& handle = left ? moved.left : moved.right;
    glm::vec3& other = left ? moved.right : moved.left;

    if (type == Handle::Vector) {
        type = Handle::Free;
    } else if (type == Handle::Automatic) {
        type = Handle::Aligned;
        if (otherType == Handle::Automatic) {
            otherType = Handle::Aligned;
        }
    }
    handle = position;

    if (type != Handle::Aligned || otherType != Handle::Aligned) {
        return;
    }
    const glm::vec3 away = moved.position - handle;
    const float awayLength = glm::length(away);
    if (awayLength <= 0.0F) {
        return;
    }
    other = moved.position + (away * (glm::length(other - moved.position) / awayLength));
}

};  // namespace v3d::curve

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/internal/Wave.h>

#include <moya/libmoya/kernel/geometry/Motion.h>

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::accel::internal {

namespace {

/** How many times a range is halved around a place the slope may be zero. **/
constexpr int kDeepest = 16;

/** A rotation this near none turns nothing, and its axis is not found. **/
constexpr double kStill = 1.0e-12;

};  // namespace

double Wave::at(double w) const {
    const double angle = theta * w;
    return a0 + (a1 * w) + ((b0 + (b1 * w)) * std::cos(angle)) + ((c0 + (c1 * w)) * std::sin(angle));
}

double Wave::slope(double w) const {
    const double angle = theta * w;
    return a1 + ((b1 + (theta * c0) + (theta * c1 * w)) * std::cos(angle)) + ((c1 - (theta * b0) - (theta * b1 * w)) * std::sin(angle));
}

double Wave::bend() const {
    // the slope's slope, with every cosine and sine at its largest and w at one
    return (2.0 * theta * (std::fabs(b1) + std::fabs(c1))) + (theta * theta * (std::fabs(b0) + std::fabs(b1) + std::fabs(c0) + std::fabs(c1)));
}

double Wave::greatest(double from, double to) const {
    double best = std::max(at(from), at(to));
    const double most = bend();
    if (most > 0.0 && to > from) {
        search(from, to, most, 0, best);
    }
    return best;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the range, then the slope's bound
void Wave::search(double lower, double upper, double bend, int depth, double & best) const {
    const double middle = (lower + upper) * 0.5;
    const double half = (upper - lower) * 0.5;
    const double tilt = slope(middle);
    // the slope moves by at most bend times the distance, so it cannot reach zero in the range
    if (std::fabs(tilt) > bend * half) {
        return;
    }
    if (depth == kDeepest) {
        // the value over the range lies under its tangent at the middle and the bend's parabola
        best = std::max(best, at(middle) + (std::fabs(tilt) * half) + (bend * half * half * 0.5));
        return;
    }
    search(lower, middle, bend, depth + 1, best);
    search(middle, upper, bend, depth + 1, best);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the line's value, then its rise
Wave Wave::overLine(double value, double rise) const {
    Wave out = *this;
    out.a0 -= value;
    out.a1 -= rise;
    return out;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the line's value, then its rise
Wave Wave::underLine(double value, double rise) const {
    Wave out;
    out.a0 = value - a0;
    out.a1 = rise - a1;
    out.b0 = -b0;
    out.b1 = -b1;
    out.c0 = -c0;
    out.c1 = -c1;
    out.theta = theta;
    return out;
}

Wave Wave::carried(const kernel::geometry::Key & from, const kernel::geometry::Key & to, const glm::dvec3 & point, int axis) {
    // the rotation between the keys turns by theta about an axis fixed in world space, after the
    // first key's rotation, as interpolating it the shortest way at an even speed does
    const glm::dquat first { from.rotation };
    glm::dquat turn = glm::dquat { to.rotation } * glm::conjugate(first);
    if (turn.w < 0.0) {
        turn = -turn;
    }
    const double half = std::acos(std::clamp(turn.w, -1.0, 1.0));
    const double sine = std::sin(half);
    Wave out;
    glm::dvec3 axisOf { 0.0, 0.0, 1.0 };
    if (sine > kStill) {
        out.theta = 2.0 * half;
        axisOf = glm::dvec3 { turn.x, turn.y, turn.z } / sine;
    }

    // before the turn the point moves along a line: start + w * along
    const glm::dmat3 rotation = glm::mat3_cast(first);
    const glm::dvec3 start = rotation * (glm::dmat3 { from.scale } * point);
    const glm::dvec3 along = rotation * ((glm::dmat3 { to.scale } - glm::dmat3 { from.scale }) * point);

    // turning v about the axis gives n (n . v) + (v - n (n . v)) cos + (n x v) sin
    const glm::dvec3 startAxial = axisOf * glm::dot(axisOf, start);
    const glm::dvec3 alongAxial = axisOf * glm::dot(axisOf, along);
    const glm::dvec3 startCross = glm::cross(axisOf, start);
    const glm::dvec3 alongCross = glm::cross(axisOf, along);
    out.a0 = static_cast<double>(from.translation[axis]) + startAxial[axis];
    out.a1 = static_cast<double>(to.translation[axis] - from.translation[axis]) + alongAxial[axis];
    out.b0 = start[axis] - startAxial[axis];
    out.b1 = along[axis] - alongAxial[axis];
    out.c0 = startCross[axis];
    out.c1 = alongCross[axis];
    if (out.theta == 0.0) {
        // with no turn, the cosine is one and the sine zero
        out.a0 += out.b0;
        out.a1 += out.b1;
        out.b0 = out.b1 = out.c0 = out.c1 = 0.0;
    }
    return out;
}

};  // namespace v3d::moya::accel::internal

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Motion.h>

#include <glm/vec3.hpp>

namespace v3d::moya::accel::internal {

/**
 * One coordinate of a point carried between two keys, as a function of how far along the segment
 * it is, from 0 to 1:
 *
 *     a0 + a1 w + (b0 + b1 w) cos(theta w) + (c0 + c1 w) sin(theta w)
 *
 * The translation and the scale move the point along a line, and the rotation turns that line
 * about a fixed axis by theta over the segment. A line added to or taken from the coordinate keeps
 * this form.
 **/
struct Wave {
    double a0 { 0.0 };
    double a1 { 0.0 };
    double b0 { 0.0 };
    double b1 { 0.0 };
    double c0 { 0.0 };
    double c1 { 0.0 };
    double theta { 0.0 };

    double at(double w) const;

    /** The slope of the coordinate at w. **/
    double slope(double w) const;

    /**
     * The greatest value over [from, to], within [0, 1], or a bound a little above it, never
     * below. The range is halved wherever the slope may turn to zero, by a bound on how fast the
     * slope changes, and the value near each such place is bounded from above.
     **/
    double greatest(double from, double to) const;

    /**
     * How far the coordinate rises over a line, which passes through value at w = 0 and rises by
     * rise to w = 1.
     **/
    Wave overLine(double value, double rise) const;

    /** How far the coordinate falls under a line, as overLine() names it. **/
    Wave underLine(double value, double rise) const;

    /** One coordinate, along an axis, of a point carried from one key to the next. **/
    static Wave carried(const kernel::geometry::Key & from, const kernel::geometry::Key & to, const glm::dvec3 & point, int axis);

 private:
    /** A bound on the slope's own slope over [0, 1], from the coefficients. **/
    double bend() const;

    /** Raises best to the greatest value over [lower, upper] wherever the slope may be zero. **/
    void search(double lower, double upper, double bend, int depth, double & best) const;
};

};  // namespace v3d::moya::accel::internal

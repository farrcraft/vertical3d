/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <vector>

#include "Curve.h"

#include <glm/vec3.hpp>

namespace v3d::curve {

/**
 * How a basis curve's points shape it, as RI's Curves and USD's BasisCurves name them.
 **/
enum class Basis : std::uint8_t {
    Linear,     /**< straight lines through every point **/
    Bezier,     /**< cubic, by three points per segment, passing through every third **/
    CatmullRom  /**< cubic, through every point but the first and the last **/
};

/**
 * A curve in the form a renderer takes: one list of points under one basis.
 *
 * A periodic curve joins its end to its start. A periodic Bezier curve has three points per
 * segment, and the last segment ends on the first point. A nonperiodic one has three per
 * segment and one more.
 **/
struct BasisCurve final {
    Basis basis = Basis::Linear;
    bool periodic = false;
    std::vector<glm::vec3> points;

    bool operator==(const BasisCurve& other) const = default;
};

/**
 * A curve as a basis curve.
 *
 * Poly, Bezier and Catmull-Rom curves convert exactly. An open Catmull-Rom curve gains a copy of
 * each end point beyond the end, so that it still starts and ends there. A NURBS curve of order
 * 2 converts exactly to a linear curve on its points. A NURBS curve of order 3 or 4 whose weights
 * are all equal converts exactly to a Bezier curve with one segment per span. Any other NURBS
 * curve is approximated by a Bezier curve with pieces segments per span, each matching the
 * curve's position and direction at its ends.
 *
 * @param curve one that problem() finds nothing wrong with
 * @param pieces at least 1
 **/
BasisCurve toBasis(const Curve& curve, unsigned int pieces);

/**
 * Whether toBasis() gives exactly the curve, not an approximation.
 **/
bool exact(const Curve& curve);

/**
 * The points a basis curve passes through, with each segment cut into resolution pieces, as
 * evaluate() cuts a curve. A periodic curve ends with its first point again.
 *
 * @param curve one with at least two points, and for Bezier the count its periodic flag needs
 * @param resolution at least 1
 **/
std::vector<glm::vec3> evaluate(const BasisCurve& curve, unsigned int resolution);

};  // namespace v3d::curve

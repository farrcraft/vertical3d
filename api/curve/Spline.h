/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <vector>

#include "Curve.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::curve {

/**
 * A NURBS curve with its knots worked out and its closure unrolled, ready to evaluate.
 *
 * Each point is homogeneous: its position times its weight, then the weight. A closed curve
 * repeats its first degree points at the end, over uniform knots, so it evaluates as an open
 * one whose end meets its start.
 **/
class Spline final {
 public:
    /**
     * @param nurbs a curve that problem() finds nothing wrong with
     **/
    Spline(const Nurbs& nurbs, bool closed);

    unsigned int degree(void) const noexcept;

    /**
     * Every span of the domain that has length, in order, by the index of the knot that starts
     * it. A span runs from knot(span) to knot(span + 1).
     **/
    std::vector<std::size_t> spans(void) const;

    float knot(std::size_t index) const;

    /**
     * The point at u, by the polynomial of the given span, so a span's end is reached from
     * inside it.
     **/
    glm::vec3 point(std::size_t span, float u) const;

    /**
     * How fast the point moves per unit of u at u, by the given span's polynomial.
     **/
    glm::vec3 derivative(std::size_t span, float u) const;

    /**
     * Whether every weight is the same, so that the curve is a polynomial in each span.
     **/
    bool polynomial(void) const noexcept;

 private:
    glm::vec4 homogeneous(std::size_t span, float u) const;
    glm::vec4 homogeneousDerivative(std::size_t span, float u) const;

    std::vector<glm::vec4> points_;
    std::vector<float> knots_;
    unsigned int degree_;
    bool polynomial_;
};

};  // namespace v3d::curve

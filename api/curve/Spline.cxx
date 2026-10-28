/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Spline.h"

#include <cstddef>
#include <vector>

#include "Curve.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::curve {

namespace {

/**
 * de Boor's algorithm over the degree + 1 points that shape one span.
 *
 * local holds the points span - degree to span, and knots is indexed as the points are.
 **/
glm::vec4 deBoor(std::vector<glm::vec4> local, const float* knots, std::size_t span, unsigned int degree, float u) {
    for (unsigned int level = 1; level <= degree; level++) {
        for (unsigned int j = degree; j >= level; j--) {
            const std::size_t index = span - degree + j;
            const float alpha = (u - knots[index]) / (knots[index + degree + 1 - level] - knots[index]);
            local[j] = (local[j - 1] * (1.0F - alpha)) + (local[j] * alpha);
        }
    }
    return local[degree];
}

};  // namespace

Spline::Spline(const Nurbs& nurbs, bool closed) : degree_(nurbs.order - 1), polynomial_(true) {
    const std::size_t count = nurbs.points.size();
    const std::size_t total = closed ? count + degree_ : count;
    points_.reserve(total);
    for (std::size_t each = 0; each < total; each++) {
        const std::size_t index = each % count;
        const float weight = nurbs.weights.empty() ? 1.0F : nurbs.weights[index];
        if (weight != (nurbs.weights.empty() ? 1.0F : nurbs.weights.front())) {
            polynomial_ = false;
        }
        points_.emplace_back(nurbs.points[index] * weight, weight);
    }

    const std::size_t knots = total + nurbs.order;
    if (!nurbs.knots.empty()) {
        knots_ = nurbs.knots;
    } else if (closed) {
        for (std::size_t each = 0; each < knots; each++) {
            knots_.push_back(static_cast<float>(each));
        }
    } else {
        // clamped: order knots at each end, so the curve starts and ends on its end points
        const std::size_t inner = total - nurbs.order;
        for (std::size_t each = 0; each < knots; each++) {
            if (each < nurbs.order) {
                knots_.push_back(0.0F);
            } else if (each < nurbs.order + inner) {
                knots_.push_back(static_cast<float>(each - nurbs.order + 1));
            } else {
                knots_.push_back(static_cast<float>(inner + 1));
            }
        }
    }
}

unsigned int Spline::degree(void) const noexcept {
    return degree_;
}

std::vector<std::size_t> Spline::spans(void) const {
    std::vector<std::size_t> spans;
    for (std::size_t span = degree_; span < points_.size(); span++) {
        if (knots_[span] < knots_[span + 1]) {
            spans.push_back(span);
        }
    }
    return spans;
}

float Spline::knot(std::size_t index) const {
    return knots_[index];
}

glm::vec3 Spline::point(std::size_t span, float u) const {
    const glm::vec4 point = homogeneous(span, u);
    return glm::vec3(point) / point.w;
}

glm::vec3 Spline::derivative(std::size_t span, float u) const {
    // the quotient rule: C = A / w, so C' = (A' - w' C) / w
    const glm::vec4 point = homogeneous(span, u);
    const glm::vec4 slope = homogeneousDerivative(span, u);
    const glm::vec3 position = glm::vec3(point) / point.w;
    return (glm::vec3(slope) - (slope.w * position)) / point.w;
}

bool Spline::polynomial(void) const noexcept {
    return polynomial_;
}

glm::vec4 Spline::homogeneous(std::size_t span, float u) const {
    const std::vector<glm::vec4> local(points_.begin() + static_cast<std::ptrdiff_t>(span - degree_),
        points_.begin() + static_cast<std::ptrdiff_t>(span + 1));
    return deBoor(local, knots_.data(), span, degree_, u);
}

glm::vec4 Spline::homogeneousDerivative(std::size_t span, float u) const {
    // The derivative of a B-spline of degree p is one of degree p - 1, over the same knots less
    // the first and last, whose points are p (P[i+1] - P[i]) / (t[i+p+1] - t[i+1]).
    const float scale = static_cast<float>(degree_);
    std::vector<glm::vec4> local;
    local.reserve(degree_);
    for (std::size_t i = span - degree_; i < span; i++) {
        local.push_back(scale * (points_[i + 1] - points_[i]) / (knots_[i + degree_ + 1] - knots_[i + 1]));
    }
    if (degree_ == 1) {
        return local.front();
    }
    return deBoor(local, knots_.data() + 1, span - 1, degree_ - 1, u);
}

};  // namespace v3d::curve

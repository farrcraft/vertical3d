/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <vector>

#include <glm/vec3.hpp>

namespace v3d::curve {

/**
 * A non-uniform rational B-spline.
 *
 * order is the degree plus one, so 2 is straight lines and 4 is cubic. weights is either empty,
 * when every point weighs 1, or one positive weight per point. knots is either empty or holds
 * the number of points plus the order, never falling.
 *
 * Empty knots are worked out. An open curve's are uniform and clamped, so the curve starts on
 * its first point and ends on its last. A closed curve's are uniform, and a closed curve cannot
 * be given knots of its own.
 **/
struct Nurbs final {
    static constexpr unsigned int CUBIC = 4;

    std::vector<glm::vec3> points;
    std::vector<float> weights;
    std::vector<float> knots;
    unsigned int order = CUBIC;

    bool operator==(const Nurbs& other) const = default;
};

};  // namespace v3d::curve

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/common.hpp>
#include <glm/vec2.hpp>

namespace positions {

/**
 * A position along a line, which can be drawn between two steps.
 **/
struct Position1D final {
    float value = 0.0F;
};

inline Position1D interpolate(const Position1D& from, const Position1D& to, float alpha) {
    return Position1D{glm::mix(from.value, to.value, alpha)};
}

/**
 * A position on a plane, which can be drawn between two steps.
 **/
struct Position2D final {
    glm::vec2 value{0.0F};
};

inline Position2D interpolate(const Position2D& from, const Position2D& to, float alpha) {
    return Position2D{glm::mix(from.value, to.value, alpha)};
}

};  // namespace positions

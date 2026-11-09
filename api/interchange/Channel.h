/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <glm/vec4.hpp>

namespace v3d::interchange {

/**
 * The keyed values of one part of one node's placement.
 *
 * A value is a vec4 whatever the part: x, y and z for a translation or a scale, and a
 * quaternion's x, y, z and w for a rotation. Cubic spline keys also carry a tangent on each
 * side, in value per second.
 **/
struct Channel {
    enum class Path : std::uint8_t { Translation, Rotation, Scale };
    enum class Interpolation : std::uint8_t { Step, Linear, CubicSpline };

    std::size_t node = 0;
    Path path = Path::Translation;
    Interpolation interpolation = Interpolation::Linear;
    /**
     * The key times in seconds, rising.
     **/
    std::vector<float> times;
    std::vector<glm::vec4> values;
    std::vector<glm::vec4> inTangents;
    std::vector<glm::vec4> outTangents;
};

};  // namespace v3d::interchange

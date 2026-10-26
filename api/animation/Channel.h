/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec4.hpp>

namespace v3d::animation {

/**
 * One joint's translation, rotation or scale, keyed in time - glTF's animation channel, kept as
 * the file gave it.
 **/
struct Channel final {
    enum class Path : std::uint8_t {
        Translation,
        Rotation,
        Scale
    };

    enum class Interpolation : std::uint8_t {
        Step,         /**< each key held until the next **/
        Linear,       /**< lerped, or slerped the short way round for a rotation **/
        CubicSpline   /**< Hermite, with an in and an out tangent a key **/
    };

    uint16_t joint{ 0 };  /**< into the skeleton's joints **/
    Path path{ Path::Translation };
    Interpolation interpolation{ Interpolation::Linear };

    /**
     * The key times in seconds, rising.
     **/
    std::vector<float> times;

    /**
     * One value per key, or three per key for a cubic spline: the in tangent, the value and the
     * out tangent. A translation or a scale is xyz, and a rotation a quaternion in x, y, z, w
     * order.
     **/
    std::vector<glm::vec4> values;
};

};  // namespace v3d::animation

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <string>

#include <glm/vec3.hpp>

namespace v3d::interchange {

/**
 * A light. A distant or spot light shines along its node's -z axis.
 **/
struct Light {
    enum class Type : std::uint8_t { Point, Distant, Spot };

    std::string name;
    Type type = Type::Point;
    glm::vec3 colour{ 1.0F };
    /**
     * The power, in watts per steradian for a point or spot light and watts per square metre
     * for a distant one. A reader of a photometric format divides by 683 lumens per watt.
     **/
    float intensity = 1.0F;
    /**
     * A spot light's half angles: where its light ends, and where it starts to fade toward
     * that edge, in radians.
     **/
    float outerCone = 0.0F;
    float innerCone = 0.0F;
};

};  // namespace v3d::interchange

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <string>

#include <glm/vec2.hpp>

namespace v3d::interchange {

/**
 * A camera. It looks along its node's -z axis, with +y up.
 **/
struct Camera {
    enum class Projection : std::uint8_t { Perspective, Orthographic };

    std::string name;
    Projection projection = Projection::Perspective;
    /**
     * A perspective camera's full angle of view from the bottom of the picture to the top, in
     * radians.
     **/
    float yfov = 0.0F;
    /**
     * The picture's width over its height, or zero when the file does not say.
     **/
    float aspect = 0.0F;
    /**
     * Half the width and half the height of what an orthographic camera sees, in its node's
     * units.
     **/
    glm::vec2 magnification{ 0.0F };
    /**
     * The near and far clipping distances along the view direction, in that order. The far one
     * is zero when the camera has no far plane.
     **/
    glm::vec2 clipping{ 0.0F };
};

};  // namespace v3d::interchange

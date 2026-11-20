/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>

#include <glm/vec2.hpp>

namespace v3d::editor::view {

/**
 * One end of a world axis, as the marker or the gizmo draws it.
 **/
struct AxisEnd final {
    std::size_t axis = 0;      /**< 0, 1 or 2 for x, y and z **/
    bool positive = true;      /**< whether this is the axis's positive end **/
    glm::vec2 point{ 0.0F };   /**< where it is drawn, in window pixels **/
    float depth = 0.0F;        /**< how far it points toward the viewer, from -1 to 1 **/
};

};  // namespace v3d::editor::view

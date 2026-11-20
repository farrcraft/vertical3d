/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/ext/quaternion_float.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor::manipulator {

/**
 * One motion event's worth of a manipulator's drag, about a pivot.
 *
 * A point moves by stretching along the frame's axes about the pivot, then turning about the
 * pivot, then moving by the offset. Each manipulator sets only its own part and leaves the
 * others at rest.
 **/
struct Motion {
    glm::vec3 offset{0.0F, 0.0F, 0.0F};
    glm::quat turn{1.0F, 0.0F, 0.0F, 0.0F};
    /**
     * How much to stretch along each axis of the frame. An object's own scale is
     * multiplied by it in the object's own axes.
     **/
    glm::vec3 stretch{1.0F, 1.0F, 1.0F};
};

};  // namespace v3d::editor::manipulator

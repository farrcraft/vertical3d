/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/dag/Transform.h>

#include <glm/ext/quaternion_float.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 * Where a mesh is, all three parts of it at once.
 *
 * A manipulator writes one of translation, rotation and scale on the object's transform.
 * A gesture is undone by restoring the whole placement, so the record does not depend on
 * which manipulator made it.
 **/
struct Placement {
    glm::vec3 translation{ 0.0f, 0.0f, 0.0f };
    glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

    /**
     **/
    static Placement of(const v3d::dag::Transform& transform);

    /**
     **/
    void applyTo(v3d::dag::Transform* transform) const;

    /**
     * @return whether the two are the same placement to within a tolerance
     *
     * A gesture that measured nothing still composes a rotation and adds a zero offset,
     * so the two ends of it differ in the last bits without the object having moved.
     **/
    bool same(const Placement& other) const;
};

};  // namespace v3d::editor

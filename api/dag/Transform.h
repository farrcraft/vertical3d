/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/type/Transform.h>

#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>

namespace v3d::dag {

/**
 * A translation, a rotation and a scale, and the matrix they compose to.
 *
 * What derives from this owns a placement in the world rather than a position: a mesh
 * describes its geometry about its own origin and is drawn through matrix().
 **/
class Transform {
 public:
    Transform();
    virtual ~Transform();

    void scale(const glm::vec3 & s);
    void rotation(const glm::quat & r);
    void translation(const glm::vec3 & t);

    glm::vec3 scale(void) const;
    glm::quat rotation(void) const;
    glm::vec3 translation(void) const;

    /**
     * Move by an offset, rather than to a position - what a drag of a translate
     * manipulator does, where the gesture measures a delta and not a destination.
     **/
    void translate(const glm::vec3 & offset);

    /**
     * @return the composition, translation * rotation * scale - type::Transform::matrix()
     **/
    glm::mat4 matrix(void) const;

    /**
     * @return all three parts at once, as the value an ecs entity is placed by
     **/
    const v3d::type::Transform & value(void) const;

 private:
    v3d::type::Transform value_;
};

};  // namespace v3d::dag

/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/type/Transform.h>

#include <cstdint>

#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>

namespace v3d::dag {

/**
 * A translation, a rotation and a scale, and the matrix they compose to.
 *
 * A class that derives from this is placed in the world by matrix(). A mesh, for example,
 * describes its geometry about its own origin and is drawn through matrix().
 *
 * It counts its changes: every call to a setter adds one to changes(), so a caller that kept
 * the count can tell whether the transform has been set since.
 **/
class Transform {
 public:
    Transform();
    virtual ~Transform();

    void scale(const glm::vec3 & s);
    void rotation(const glm::quat & orientation);
    void translation(const glm::vec3 & t);

    glm::vec3 scale(void) const;
    glm::quat rotation(void) const;
    glm::vec3 translation(void) const;

    /**
     * Adds offset to the translation. A drag of the translate manipulator measures a
     * delta, not a destination, so it moves the mesh through this.
     **/
    void translate(const glm::vec3 & offset);

    /**
     * @return translation * rotation * scale, composed by type::Transform::matrix()
     **/
    glm::mat4 matrix(void) const;

    /**
     * @return the translation, rotation and scale as one type::Transform, the type an ecs
     *         entity is placed by
     **/
    const v3d::type::Transform & value(void) const;

    /**
     * @return how many times a setter has been called. It never goes down
     **/
    std::uint64_t changes(void) const noexcept;

 private:
    v3d::type::Transform value_;
    std::uint64_t changes_ = 0;
};

};  // namespace v3d::dag

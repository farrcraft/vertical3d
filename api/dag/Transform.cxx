/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Transform.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace v3d::dag {

    Transform::Transform() :
        translation_(0.0f, 0.0f, 0.0f),
        scale_(1.0f, 1.0f, 1.0f),
        rotation_(1.0f, 0.0f, 0.0f, 0.0f) {
    }

    Transform::~Transform() {
    }

    void Transform::scale(const glm::vec3& s) {
        scale_ = s;
    }

    void Transform::rotation(const glm::quat& r) {
        rotation_ = r;
    }

    void Transform::translation(const glm::vec3& t) {
        translation_ = t;
    }

    void Transform::translate(const glm::vec3& offset) {
        translation_ += offset;
    }

    glm::vec3 Transform::scale(void) const {
        return scale_;
    }

    glm::quat Transform::rotation(void) const {
        return rotation_;
    }

    glm::vec3 Transform::translation(void) const {
        return translation_;
    }

    glm::mat4 Transform::matrix(void) const {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation_);
        transform *= glm::mat4_cast(rotation_);
        transform = glm::scale(transform, scale_);
        return transform;
    }

};  // namespace v3d::dag

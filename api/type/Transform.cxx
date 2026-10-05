/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Transform.h"

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace v3d::type {

glm::mat4 Transform::matrix() const {
    return glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation) *
        glm::scale(glm::mat4(1.0f), scale);
}

Transform interpolate(const Transform& from, const Transform& to, const float alpha) {
    Transform blended;
    blended.position = glm::mix(from.position, to.position, alpha);
    blended.rotation = glm::slerp(from.rotation, to.rotation, alpha);
    blended.scale = glm::mix(from.scale, to.scale, alpha);
    return blended;
}

};  // namespace v3d::type

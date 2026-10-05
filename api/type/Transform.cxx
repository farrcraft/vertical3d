/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Transform.h"

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace v3d::type {

glm::mat4 Transform::matrix() const {
    // a rotation built up by multiplying many turns drifts off unit length, and a quaternion
    // that is not unit length scales and shears as well as turning, so it is normalised here.
    // One of no length names no rotation at all
    const float length = glm::length(rotation);
    const glm::quat turn = length > 0.0f ? rotation / length : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    return glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(turn) *
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

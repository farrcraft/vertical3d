/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Transform.h"

#include <cmath>

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace v3d::type {

glm::mat4 Transform::matrix() const {
    // a rotation built up by multiplying many turns drifts off unit length. A quaternion that
    // is not unit length scales as well as turning, so it is normalised here. A quaternion of
    // zero length, or of no finite length, is treated as no rotation
    const float length = glm::length(rotation);
    const glm::quat turn = std::isfinite(length) && length > 0.0F ? rotation / length : glm::quat(1.0F, 0.0F, 0.0F, 0.0F);
    return glm::translate(glm::mat4(1.0F), position) * glm::mat4_cast(turn) *
        glm::scale(glm::mat4(1.0F), scale);
}

Transform interpolate(const Transform& from, const Transform& to, const float alpha) {
    Transform blended;
    blended.position = glm::mix(from.position, to.position, alpha);
    blended.rotation = glm::slerp(from.rotation, to.rotation, alpha);
    blended.scale = glm::mix(from.scale, to.scale, alpha);
    return blended;
}

};  // namespace v3d::type

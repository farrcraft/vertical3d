/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Transform.h"

#include <cstdint>

#include <glm/gtc/quaternion.hpp>

namespace v3d::dag {

Transform::Transform() {
}

Transform::~Transform() {
}

void Transform::scale(const glm::vec3& s) {
    value_.scale = s;
    changes_++;
}

void Transform::rotation(const glm::quat& orientation) {
    value_.rotation = orientation;
    changes_++;
}

void Transform::translation(const glm::vec3& t) {
    value_.position = t;
    changes_++;
}

void Transform::translate(const glm::vec3& offset) {
    value_.position += offset;
    changes_++;
}

glm::vec3 Transform::scale(void) const {
    return value_.scale;
}

glm::quat Transform::rotation(void) const {
    return value_.rotation;
}

glm::vec3 Transform::translation(void) const {
    return value_.position;
}

glm::mat4 Transform::matrix(void) const {
    return value_.matrix();
}

const v3d::type::Transform& Transform::value(void) const {
    return value_;
}

std::uint64_t Transform::changes(void) const noexcept {
    return changes_;
}

};  // namespace v3d::dag

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::geometry {

/**
 * One sample of a transformation, decomposed into a translation, a rotation and a symmetric
 * scale, which holds any shear. The transformation is the translation, then the rotation, then the
 * scale, applied to a point from the right.
 **/
struct Key {
    float time { 0.0F };
    glm::vec3 translation { 0.0F };
    glm::quat rotation { 1.0F, 0.0F, 0.0F, 0.0F };
    glm::mat3 scale { 1.0F };
};

};  // namespace v3d::moya::kernel::geometry

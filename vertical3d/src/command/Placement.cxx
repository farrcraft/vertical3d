/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Placement.h"

#include <api/type/Transform.h>

#include <cmath>

#include <glm/ext/quaternion_float.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

namespace {

/**
 * How far apart two placements may be and still be the same one. A drag is measured
 * in world units of a scene whose primitives are one unit across.
 **/
const float tolerance = 1e-6F;

/**
 **/
bool close(float first, float second) {
    return std::fabs(first - second) <= tolerance;
}

/**
 **/
bool close(const glm::vec3& first, const glm::vec3& second) {
    return close(first.x, second.x) && close(first.y, second.y) && close(first.z, second.z);
}

/**
 **/
bool close(const glm::quat& first, const glm::quat& second) {
    return close(first.w, second.w) && close(first.x, second.x) && close(first.y, second.y) && close(first.z, second.z);
}

};  // namespace

/**
 **/
bool same(const v3d::type::Transform& first, const v3d::type::Transform& second) {
    return close(first.position, second.position) &&
        close(first.rotation, second.rotation) &&
        close(first.scale, second.scale);
}

};  // namespace v3d::editor

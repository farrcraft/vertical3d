/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Placement.h"

#include <api/type/Transform.h>

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>

namespace v3d::interchange {

v3d::type::Transform placementOf(const glm::mat4& matrix) {
    v3d::type::Transform placement;
    placement.position = glm::vec3(matrix[3]);
    glm::mat3 basis(matrix);
    placement.scale = glm::vec3(glm::length(basis[0]), glm::length(basis[1]), glm::length(basis[2]));
    if (glm::determinant(basis) < 0.0F) {
        placement.scale.x = -placement.scale.x;
    }
    for (glm::length_t axis = 0; axis < 3; axis++) {
        if (placement.scale[axis] != 0.0F) {
            basis[axis] /= placement.scale[axis];
        }
    }
    placement.rotation = glm::normalize(glm::quat_cast(basis));
    return placement;
}

};  // namespace v3d::interchange

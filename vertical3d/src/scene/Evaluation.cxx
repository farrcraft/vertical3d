/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Evaluation.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 **/
Placed Evaluation::placed(unsigned int id) const {
    const auto found = placed_.find(id);
    return found == placed_.end() ? Placed() : found->second;
}

/**
 **/
void Evaluation::place(unsigned int id, const Placed& placed) {
    placed_[id] = placed;
}

/**
 **/
glm::quat orientation(const glm::mat4& matrix) {
    const glm::vec3 x = glm::normalize(glm::vec3(matrix[0]));
    const glm::vec3 along = glm::vec3(matrix[1]);
    const glm::vec3 y = glm::normalize(along - (glm::dot(along, x) * x));
    const glm::vec3 z = glm::cross(x, y);
    return glm::normalize(glm::quat_cast(glm::mat3(x, y, z)));
}

/**
 **/
glm::mat4 withoutScale(const glm::mat4& matrix) {
    return glm::translate(glm::mat4(1.0F), glm::vec3(matrix[3])) * glm::mat4_cast(orientation(matrix));
}

};  // namespace v3d::editor

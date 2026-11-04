/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Evaluation.h"

#include <api/type/Transform.h>
#include <vertical3d/src/scene/item/Item.h>

#include "Properties.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor::scene {

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
Pose Evaluation::pose(const item::Item& item) const {
    const auto found = poses_.find(item.id());
    return found == poses_.end() ? scene::pose(item) : found->second;
}

/**
 **/
void Evaluation::pose(unsigned int id, const Pose& pose) {
    poses_[id] = pose;
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

/**
 **/
v3d::type::Transform placement(const glm::mat4& matrix) {
    v3d::type::Transform taken;
    taken.position = glm::vec3(matrix[3]);
    taken.rotation = orientation(matrix);
    const glm::mat3 stretch = glm::transpose(glm::mat3_cast(taken.rotation)) * glm::mat3(matrix);
    taken.scale = glm::vec3(stretch[0][0], stretch[1][1], stretch[2][2]);
    return taken;
}

};  // namespace v3d::editor::scene

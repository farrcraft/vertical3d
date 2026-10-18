/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Transform.h"

#include <glm/gtc/matrix_transform.hpp>

namespace v3d::ecs::component {

glm::quat aboutY(const float radians) {
    return glm::angleAxis(radians, glm::vec3(0.0F, 1.0F, 0.0F));
}

};  // namespace v3d::ecs::component

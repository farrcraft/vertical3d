/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec3.hpp>

namespace v3d::ecs::component {

/**
 * A thing's colour, red, green and blue. An aggregate, so it is copied and written directly.
 **/
struct Color3 final {
    glm::vec3 value{1.0f};
};

};  // namespace v3d::ecs::component

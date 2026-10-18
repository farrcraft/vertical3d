/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec3.hpp>

namespace v3d::type::geometry {

/**
 * A point and a direction, named where they are given so the two cannot be passed in the wrong
 * order:
 *
 *     Ray ray({.origin = eye, .direction = forward});
 *
 * The direction keeps whatever length it is given. Ray normalises it.
 **/
struct Line {
    glm::vec3 origin{0.0F};
    glm::vec3 direction{0.0F, 0.0F, 1.0F};
};

};  // namespace v3d::type::geometry

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/mat4x4.hpp>

namespace v3d::editor::scene {

/**
 * Where one item is, as an evaluation of its scene found it.
 *
 * The local matrix is measured in the parent's space, and the world matrix is the parent's
 * world matrix times the local one. An item with no parent has the identity as its parent's
 * world matrix.
 **/
struct Placed {
    glm::mat4 parent{ 1.0F };
    glm::mat4 local{ 1.0F };
    glm::mat4 world{ 1.0F };
};

};  // namespace v3d::editor::scene

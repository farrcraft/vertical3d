/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec3.hpp>

namespace v3d::curve::internal {

/**
 * The points either side of a handle's point: toward is the one on the handle's side, and
 * across the one on the other side. Either is null at an open curve's end.
 **/
struct Neighbours {
    const glm::vec3* toward = nullptr;
    const glm::vec3* across = nullptr;
};

};  // namespace v3d::curve::internal

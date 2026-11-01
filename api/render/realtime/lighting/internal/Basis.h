/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec3.hpp>

namespace v3d::render::realtime::lighting::internal {

/**
 * The light's own axes: its right, its up, and the direction it looks along, which is +z of its
 * basis as a camera's is.
 **/
struct Basis final {
    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 along;
};

};  // namespace v3d::render::realtime::lighting::internal

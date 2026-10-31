/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>

#include <glm/vec3.hpp>

namespace v3d::render::realtime::lighting::internal {

/**
 * The camera's view volume: the four edges from a near corner to the far corner behind it, and
 * the view direction from the middle of the near plane.
 **/
struct Volume final {
    std::array<glm::vec3, 4> near;
    std::array<glm::vec3, 4> far;
    glm::vec3 centre;  /**< the middle of the near plane **/
    glm::vec3 axis;    /**< the view direction, normalised **/
};

};  // namespace v3d::render::realtime::lighting::internal

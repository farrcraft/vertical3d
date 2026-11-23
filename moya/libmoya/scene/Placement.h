/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::scene {

/**
 * A placement at one point, turned so that its -z looks towards another, as UsdLux's lights
 * emit. Its y is as near the world's y as the direction allows, and is the world's x when the
 * direction is nearly straight up or down. With no distance between the points, the placement
 * only moves to the first.
 **/
glm::mat4 aimed(const glm::vec3 & from, const glm::vec3 & to);

};  // namespace v3d::moya::scene

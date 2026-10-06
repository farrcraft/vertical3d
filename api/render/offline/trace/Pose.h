/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/mat4x4.hpp>

namespace v3d::render::offline::trace {

/**
 * Where a moving primitive is at a ray's time: the transform from there back to the pose it
 * is stored in, and the transform forward again. Both null for a primitive that does not move.
 **/
class Pose final {
 public:
    const glm::mat4x4* ahead = nullptr;
    const glm::mat4x4* backward = nullptr;
};

};  // namespace v3d::render::offline::trace

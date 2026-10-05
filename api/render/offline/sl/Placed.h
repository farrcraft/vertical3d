/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Instance.h"

#include <glm/mat4x4.hpp>

namespace v3d::render::offline::sl {

/**
 * A shader instance and the space it was instanced in.
 *
 * RI says a shader's own space is the transform that was in force when a scene named it,
 * and that is what its `point "shader" (0, 0, 1)` and every position a scene binds are
 * stated against. The two travel together everywhere - a surface on a primitive, a light
 * in a scene - which is why they are one thing rather than two fields repeated in each
 * renderer.
 **/
class Placed final {
 public:
    InstancePtr shader;
    glm::mat4x4 placement = glm::mat4x4(1.0f);
};

};  // namespace v3d::render::offline::sl

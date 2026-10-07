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
 * RI defines a shader's own space as the transform that was in force when a scene named
 * it. Its `point "shader" (0, 0, 1)` and every position a scene binds are stated in that
 * space. The two travel together everywhere, a surface on a primitive and a light in a
 * scene, so they are one type rather than two fields repeated at each use.
 **/
class Placed final {
 public:
    InstancePtr shader;
    glm::mat4x4 placement = glm::mat4x4(1.0f);
};

};  // namespace v3d::render::offline::sl

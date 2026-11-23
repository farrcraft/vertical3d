/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/vec3.hpp>

namespace v3d::render::realtime::lighting {

/**
 * A sphere a directional light's shadow map is fitted to.
 **/
struct ShadowBounds final {
    glm::vec3 centre{0.0F};
    float radius = 0.0F;
};

};  // namespace v3d::render::realtime::lighting

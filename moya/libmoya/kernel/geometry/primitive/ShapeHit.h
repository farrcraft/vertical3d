/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/vec2.hpp>

namespace v3d::moya::kernel::geometry::primitive {

/** Where a ray met a point or a quadric: how far along it, and the position on the surface. **/
struct ShapeHit {
    float distance { 0.0F };
    glm::vec2 uv { 0.0F };
};

};  // namespace v3d::moya::kernel::geometry::primitive

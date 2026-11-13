/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/vec2.hpp>

namespace v3d::moya::kernel::surface {

/** A footprint in a surface's texture coordinates: its two axes as changes in s and t. **/
struct TextureFootprint {
    glm::vec2 major { 0.0F };
    glm::vec2 minor { 0.0F };
};

};  // namespace v3d::moya::kernel::surface

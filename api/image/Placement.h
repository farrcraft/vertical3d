/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>

#include <glm/vec4.hpp>

namespace v3d::image {

/**
 * Where a sprite sits in a sheet: x, y, width and height, in the sheet's pixels from its top
 * left corner.
 **/
class Placement final {
 public:
    std::string name;
    glm::uvec4 area = glm::uvec4(0);
};

};  // namespace v3d::image

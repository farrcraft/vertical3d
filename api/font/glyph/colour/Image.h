/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <vector>

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>

namespace v3d::font::glyph::colour {

/**
 * A picture a glyph places, as a bitmap table holds one.
 **/
struct Image final {
    glm::uvec2 size{ 0 };
    std::vector<unsigned char> pixels{};  /**< red, green, blue and alpha, unpremultiplied, in rows from the top **/
    glm::vec2 min{ 0.0F };              /**< the lower left of the box it fills, in its own space **/
    glm::vec2 max{ 0.0F };
    glm::mat3 transform{ 1.0F };        /**< from its own space into em units **/
};

};  // namespace v3d::font::glyph::colour

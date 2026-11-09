/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/vec4.hpp>

namespace v3d::font::glyph::colour {

/**
 * One colour of a fill or a gradient.
 **/
struct Stop final {
    float offset = 0.0F;       /**< its place along a gradient **/
    glm::vec4 colour{ 0.0F };  /**< red, green, blue and alpha, from 0 to 1, as the font gives them **/
    bool foreground = false;   /**< whether its red, green and blue are the text's colour, its alpha times the text's **/
};

};  // namespace v3d::font::glyph::colour

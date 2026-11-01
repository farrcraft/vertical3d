/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>

#include <glm/vec2.hpp>

namespace v3d::font {

/**
 * One glyph of a shaped run, in pixels at the size the run was shaped at. y grows downwards.
 **/
struct ShapedGlyph final {
    unsigned int index = 0;     /**< the glyph's index in its run's face **/
    float advance = 0.0F;       /**< how far the pen moves after it **/
    glm::vec2 offset{ 0.0F };   /**< where it is drawn from the pen **/
    std::size_t cluster = 0;    /**< the byte offset in the string of the first character it came from **/
};

};  // namespace v3d::font

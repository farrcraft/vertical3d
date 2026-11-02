/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstddef>

#include <glm/vec2.hpp>

namespace v3d::font::internal {

/**
 * Where a colour glyph is painted: a box of whole pixels, its lower left pixel at origin, with
 * the pixel grid's y upwards.
 **/
struct PaintGrid final {
    /**
     * FreeType's outline coordinates are 26.6 fixed point, 64 to the pixel.
     **/
    static constexpr float FIXED_POINT = 64.0F;

    glm::ivec2 origin{ 0 };
    glm::ivec2 size{ 0 };
    float scale = 0.0F;  /**< pixels to the em **/
    float shift = 0.0F;  /**< how far the pen is moved right, in pixels **/

    /**
     * @return how many pixels the box holds
     **/
    std::size_t pixels() const;

    /**
     * @return the centre of a pixel, counted in rows from the top, in em units
     **/
    glm::vec2 em(int column, int row) const;

    /**
     * @return a point in em units in FreeType's units from the box's lower left
     **/
    FT_Vector device(const glm::vec2& point) const;
};

};  // namespace v3d::font::internal

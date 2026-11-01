/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "ShapedGlyph.h"

#include <boost/shared_ptr.hpp>

namespace v3d::font {

class Face;

/**
 * A stretch of text in one face, one script and one direction, shaped into glyphs.
 *
 * The glyphs are in visual order, left to right, whatever the direction. A right-to-left run's
 * first glyph came from its last characters.
 **/
struct ShapedRun final {
    enum class Direction : std::uint8_t {
        LeftToRight,
        RightToLeft
    };

    boost::shared_ptr<Face> face;
    std::string script;           /**< its ISO 15924 code, such as "Latn" or "Arab" **/
    Direction direction = Direction::LeftToRight;
    std::size_t start = 0;        /**< the byte offset in the string where it starts **/
    std::size_t end = 0;          /**< and one past where it ends **/
    std::vector<ShapedGlyph> glyphs;
    float advance = 0.0F;         /**< every glyph's advance added together **/
};

};  // namespace v3d::font

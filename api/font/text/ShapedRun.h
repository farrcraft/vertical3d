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

namespace v3d::font::face {

class Face;

};  // namespace v3d::font::face

namespace v3d::font::text {


/**
 * A stretch of text in one face, one script and one embedding level, shaped into glyphs.
 *
 * The glyphs are in visual order, left to right, whatever the direction. A right-to-left run's
 * first glyph came from its last characters.
 **/
struct ShapedRun final {
    enum class Direction : std::uint8_t {
        LeftToRight,
        RightToLeft
    };

    boost::shared_ptr<face::Face> face;
    std::string script;           /**< its ISO 15924 code, such as "Latn" or "Arab" **/
    Direction direction = Direction::LeftToRight;  /**< right to left when its level is odd **/
    std::uint8_t level = 0;       /**< its embedding level, by which a line's runs are drawn in order **/
    std::size_t start = 0;        /**< the byte offset in the string where it starts **/
    std::size_t end = 0;          /**< and one past where it ends **/
    std::vector<std::int32_t> coordinates;  /**< its face's normalized coordinates, or none at every axis's default **/
    std::vector<ShapedGlyph> glyphs;
    float advance = 0.0F;         /**< every glyph's advance added together **/
};

};  // namespace v3d::font::text

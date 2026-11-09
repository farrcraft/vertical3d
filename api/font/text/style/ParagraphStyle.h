/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/font/text/Bidi.h>

#include <cstddef>
#include <cstdint>

namespace v3d::font::text::style {

/**
 * Where a paragraph's lines sit across its width.
 **/
enum class Alignment : std::uint8_t {
    Start,   /**< the side a line begins on: the left in a paragraph read left to right **/
    End,
    Left,
    Right,
    Center
};

/**
 * How a paragraph's text is divided into lines and placed.
 **/
struct ParagraphStyle final {
    BaseDirection direction = BaseDirection::Auto;
    Alignment alignment = Alignment::Start;
    float width = 0.0F;          /**< the width lines wrap at, in pixels. At 0 or less they never wrap **/
    std::size_t maxLines = 0;    /**< the most lines shown, or 0 for no limit **/
    bool ellipsis = true;        /**< whether the last line shown ends in an ellipsis when text is cut **/
    float lineSpacing = 1.0F;    /**< a line's height, as a multiple of its faces' natural height **/

    bool operator==(const ParagraphStyle& other) const = default;
};

};  // namespace v3d::font::text::style

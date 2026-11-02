/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/face/Style.h>
#include <api/font/face/Variation.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Bidi.h"
#include "Feature.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::font::face {

class FallbackChain;

};  // namespace v3d::font::face

namespace v3d::font::text {

/**
 * A shape drawn beneath text from its glyphs' distance fields: each glyph grown by a width,
 * faded to nothing over a softness beyond that, and moved by an offset. An outline has a width,
 * a shadow an offset, and a glow a softness.
 **/
struct Effect final {
    glm::vec4 colour{ 0.0F, 0.0F, 0.0F, 1.0F };
    float width = 0.0F;            /**< how far past the glyph's edge it is whole, in pixels at the text's size. Below 0 it is inside **/
    float softness = 0.0F;         /**< how far past that it fades to nothing, in pixels **/
    glm::vec2 offset{ 0.0F };      /**< how far it is moved from its glyph, in pixels with y downwards **/

    bool operator==(const Effect& other) const = default;
};

/**
 * How a stretch of text is shaped and drawn.
 *
 * The fonts, the face style, the size, the features, the variations and the language decide its
 * glyphs. A
 * variable face's weight axis follows the face style's weight and its optical size axis the
 * size, unless a variation sets them. The colour, the decorations, the effects and the contrast
 * only decide how they are drawn, so text that changes only those is not shaped again.
 **/
struct Style final {
    /**
     * The size text is set at unless another is given, in pixels.
     **/
    static constexpr float DEFAULT_SIZE = 16.0F;

    boost::shared_ptr<face::FallbackChain> fonts;
    face::Style face;
    float size = DEFAULT_SIZE;          /**< in pixels **/
    std::vector<Feature> features;
    std::vector<face::Variation> variations;  /**< set on each variable face's axes, in this order **/
    std::string language;               /**< a BCP 47 tag, such as "ja", that picks a face's forms for the language. Empty is the shaping cache's locale **/
    glm::vec4 colour{ 1.0F };
    bool underline = false;
    bool strikethrough = false;
    std::vector<Effect> effects;    /**< drawn beneath the text in this order, the first furthest back **/
    float contrast = 0.0F;              /**< how much the glyphs' coverage is strengthened, from 0 to 1, as light text on a dark ground needs **/

    bool operator==(const Style& other) const = default;

    /**
     * @return whether text in this style has the same glyphs as in another, which may differ in
     *         its colour and decorations
     **/
    bool shapesLike(const Style& other) const;
};

/**
 * A style over the bytes from one offset to another of a text.
 **/
struct StyledRange final {
    std::size_t start = 0;
    std::size_t end = 0;
    Style style;
};

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

};  // namespace v3d::font::text

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/font/face/Style.h>
#include <api/font/face/Variation.h>

#include <api/font/text/shaping/Feature.h>

#include <string>
#include <vector>

#include "Effect.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::font::face {

class FallbackChain;

};  // namespace v3d::font::face

/**
 * How text is shaped and drawn: the style of a stretch of text, the effects drawn beneath it, and
 * the style of a paragraph.
 **/
namespace v3d::font::text::style {

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
    face::Style face{};
    float size = DEFAULT_SIZE;          /**< in pixels **/
    std::vector<shaping::Feature> features{};
    std::vector<face::Variation> variations{};  /**< set on each variable face's axes, in this order **/
    std::string language{};             /**< a BCP 47 tag, such as "ja", that picks a face's forms for the language. Empty is the shaping cache's locale **/
    glm::vec4 colour{ 1.0F };
    bool underline = false;
    bool strikethrough = false;
    std::vector<Effect> effects{};  /**< drawn beneath the text in this order, the first furthest back **/
    float contrast = 0.0F;              /**< how much the glyphs' coverage is strengthened, from 0 to 1, as light text on a dark ground needs **/

    bool operator==(const Style& other) const = default;

    /**
     * @return whether text in this style has the same glyphs as in another, which may differ in
     *         its colour and decorations
     **/
    bool shapesLike(const Style& other) const;
};

};  // namespace v3d::font::text::style

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::font::text::style {

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

};  // namespace v3d::font::text::style

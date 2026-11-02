/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

#include "GlyphStore.h"
#include "TextStyle.h"

#include <glm/vec2.hpp>

namespace v3d::font {

/**
 * The size a distance field is made at, in pixels to the em. One field serves every size its
 * glyph is drawn at.
 **/
constexpr float FIELD_SIZE = 64.0F;

/**
 * The widest spread a field is made with, in its pixels. An effect reaches at most this far
 * from the glyph's edge, half an em.
 **/
constexpr std::uint8_t MAX_SPREAD = 32;

/**
 * Make a glyph's signed distance field from its unhinted outline, at its key's size and
 * spread. Each pixel is its distance from the glyph's edge, from the spread outside it at 0 to
 * the spread inside it at 255, with the edge at the middle. The field reaches the spread past
 * the outline on every side.
 *
 * @return the field, one byte a pixel, with its top left pixel from the pen. A key of another
 *         mode or with no spread, a face that cannot be sized, or a glyph that cannot be loaded
 *         gives an empty bitmap
 **/
GlyphBitmap field(const GlyphKey& key);

/**
 * The spread is the reach with a pixel more for the effect's edge. It is rounded up to a multiple
 * of four, so that nearby reaches share one field, and is at most MAX_SPREAD.
 *
 * @param reach how far past the glyph's edge an effect reaches, in em units
 * @return the spread a field needs for it, in the field's pixels
 **/
std::uint8_t fieldSpread(float reach);

/**
 * @return the key of the field a glyph's effects are drawn from, for an effect reaching a
 *         distance past its edge in em units
 **/
GlyphKey fieldKey(const Face* face, unsigned int index, float reach);

/**
 * @param size the size the text is drawn at, in pixels
 * @return how far past a glyph's edge an effect reaches, in em units
 **/
float effectReach(const TextEffect& effect, float size);

/**
 * Where an effect's coverage is whole and where it reaches nothing, as values of a field made
 * with a spread. Coverage fades between the two, and is whole inside the first.
 *
 * @param size the size the text is drawn at, in pixels
 * @return the value at which the effect is whole, then the value at which it is gone
 **/
glm::vec2 effectRamp(const TextEffect& effect, float size, std::uint8_t spread);

};  // namespace v3d::font

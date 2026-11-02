/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

#include "ColourGlyph.h"
#include "GlyphStore.h"

#include <glm/vec4.hpp>

namespace v3d::font {

class Library;

/**
 * Paint a colour glyph at a size in pixels to the em, its pen moved right by a step of a pixel,
 * as rasterize() moves a coverage glyph.
 *
 * Each clip is rasterized by FreeType, antialiased, and a fill or an image is drawn inside all of
 * its layer's clips. Layers and groups are put down as their composites say, in the colours the
 * font gives, blended as they are encoded, as browsers blend them. A gradient's colours are
 * mixed premultiplied, as COLR asks.
 *
 * @param foreground the text's colour, red, green, blue and alpha from 0 to 1, which a layer in
 *        the text's colour takes
 * @return the glyph's colours, four bytes a pixel, unpremultiplied, with its top left pixel from
 *         the pen. An empty glyph gives an empty bitmap
 **/
GlyphBitmap paint(const ColourGlyph& glyph, const Library& library, float size, std::uint8_t subpixel, const glm::vec4& foreground);

/**
 * Paint the colour glyph a key names, read from its face in the key's palette and at its
 * coordinates.
 *
 * @return the glyph's colours, or an empty bitmap of four channels for a key of another mode, a
 *         glyph with no colour, or a face with no library
 **/
GlyphBitmap paint(const GlyphKey& key);

/**
 * @return a colour as a key holds it: red, green, blue and alpha bytes from the highest
 **/
std::uint32_t packColour(const glm::vec4& colour);

/**
 * @return a colour a key holds, as red, green, blue and alpha from 0 to 1
 **/
glm::vec4 unpackColour(std::uint32_t colour);

};  // namespace v3d::font

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

#include "GlyphStore.h"

namespace v3d::font {

/**
 * How many steps a pixel is divided into horizontally. A glyph drawn with its pen between two
 * steps is drawn at the step to its left.
 **/
constexpr std::uint8_t SUBPIXEL_STEPS = 4;

/**
 * Rasterize a glyph's coverage at its key's size, its pen moved right by its key's step of a
 * pixel. The glyph is hinted by its face's own instructions vertically, so its stems and its
 * x-height sit on whole pixels, and not horizontally, so its shape is the same at every step.
 *
 * @return the glyph's coverage at its key's coordinates, one byte a pixel, with its top left
 *         pixel from the pen. A key of another mode, a face that cannot be sized or set to the
 *         coordinates, or a glyph that cannot be loaded gives an empty bitmap
 **/
GlyphBitmap rasterize(const GlyphKey& key);

};  // namespace v3d::font

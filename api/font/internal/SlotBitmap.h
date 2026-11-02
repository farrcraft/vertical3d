/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <api/font/GlyphStore.h>

namespace v3d::font::internal {

/**
 * @return the bitmap a glyph slot was rendered into, one byte a pixel in rows from the top, with
 *         its top left pixel from the pen. A bitmap of another pixel mode gives no pixels
 **/
font::GlyphBitmap slotBitmap(const FT_GlyphSlotRec& slot);

};  // namespace v3d::font::internal

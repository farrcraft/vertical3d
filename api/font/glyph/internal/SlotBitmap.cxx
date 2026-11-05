/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SlotBitmap.h"

#include <api/font/glyph/Store.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>

#include <glm/vec2.hpp>

namespace v3d::font::glyph::internal {

font::glyph::Bitmap slotBitmap(const FT_GlyphSlotRec& slot) {
    const FT_Bitmap& bitmap = slot.bitmap;
    font::glyph::Bitmap made{ .size = glm::uvec2(bitmap.width, bitmap.rows), .bearing = glm::ivec2(slot.bitmap_left, -slot.bitmap_top) };
    if (bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) {
        return font::glyph::Bitmap{ .bearing = made.bearing };
    }
    made.pixels.resize(static_cast<std::size_t>(bitmap.width) * bitmap.rows);
    // a row may be padded, and with a negative pitch the rows are stored from the bottom up
    const auto stride = static_cast<std::size_t>(std::abs(bitmap.pitch));
    for (unsigned int row = 0; row < bitmap.rows; row++) {
        const std::size_t stored = bitmap.pitch < 0 ? bitmap.rows - 1 - row : row;
        std::copy_n(bitmap.buffer + (stored * stride), bitmap.width, made.pixels.begin() + static_cast<std::ptrdiff_t>(static_cast<std::size_t>(row) * bitmap.width));
    }
    return made;
}

};  // namespace v3d::font::glyph::internal

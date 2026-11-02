/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Rasterize.h"

#include <api/font/internal/FaceHandle.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <vector>

#include "Face.h"
#include "GlyphStore.h"

#include <glm/vec2.hpp>

namespace v3d::font {

GlyphBitmap rasterize(const GlyphKey& key) {
    if (key.face == nullptr || key.mode != GlyphMode::Coverage) {
        return GlyphBitmap();
    }
    internal::FaceHandle& handle = key.face->handle();
    if (!handle.size(key.size)) {
        return GlyphBitmap();
    }
    // the face is hinted at many times its horizontal resolution and scaled back, so hinting
    // moves it whole pixels only vertically. The step moves the scaled glyph, after hinting
    FT_Matrix matrix = {
        static_cast<FT_Fixed>((1.0 / internal::FaceHandle::HORIZONTAL_OVERSAMPLE) * 0x10000L),  // checked: a constant, 1024
        0,
        0,
        static_cast<FT_Fixed>(0x10000L)
    };
    FT_Vector step = { static_cast<FT_Pos>(key.subpixel % SUBPIXEL_STEPS) * static_cast<FT_Pos>(internal::FaceHandle::FIXED_POINT_SCALE) / SUBPIXEL_STEPS,
        0 };
    FT_Set_Transform(handle.face, &matrix, &step);
    const FT_Error error = FT_Load_Glyph(handle.face, key.index, FT_LOAD_RENDER);
    // the next font to load from the face sets its own size, but not always its own transform
    FT_Set_Transform(handle.face, &matrix, nullptr);
    if (error != 0) {
        return GlyphBitmap();
    }

    const FT_GlyphSlotRec* slot = handle.face->glyph;
    const FT_Bitmap& bitmap = slot->bitmap;
    GlyphBitmap made{ .size = glm::uvec2(bitmap.width, bitmap.rows), .bearing = glm::ivec2(slot->bitmap_left, -slot->bitmap_top) };
    if (bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) {
        return GlyphBitmap{ .bearing = made.bearing };
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

};  // namespace v3d::font

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Rasterize.h"

#include <api/font/internal/FaceHandle.h>
#include <api/font/glyph/internal/SlotBitmap.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <api/font/face/Face.h>
#include "Store.h"

namespace v3d::font::glyph {

Bitmap rasterize(const Key& key) {
    if (key.face == nullptr || key.mode != Mode::Coverage) {
        return Bitmap();
    }
    font::internal::FaceHandle& handle = key.face->handle();
    if (!handle.vary(key.coordinates) || !handle.size(key.size)) {
        return Bitmap();
    }
    // the face is hinted at many times its horizontal resolution and scaled back, so hinting
    // moves it whole pixels only vertically. The step moves the scaled glyph, after hinting
    FT_Matrix matrix = {
        static_cast<FT_Fixed>((1.0 / font::internal::FaceHandle::HORIZONTAL_OVERSAMPLE) * 0x10000L),  // checked: a constant, 1024
        0,
        0,
        static_cast<FT_Fixed>(0x10000L)
    };
    FT_Vector step = { static_cast<FT_Pos>(key.subpixel % SUBPIXEL_STEPS) * static_cast<FT_Pos>(font::internal::FaceHandle::FIXED_POINT_SCALE) / SUBPIXEL_STEPS,
        0 };
    FT_Set_Transform(handle.face, &matrix, &step);
    const FT_Error error = FT_Load_Glyph(handle.face, key.index, FT_LOAD_RENDER);
    // the next font to load from the face sets its own size, but not always its own transform
    FT_Set_Transform(handle.face, &matrix, nullptr);
    if (error != 0) {
        return Bitmap();
    }

    return internal::slotBitmap(*handle.face->glyph);
}

};  // namespace v3d::font::glyph

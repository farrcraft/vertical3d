/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Rasterize.h"

#include <api/font/internal/FaceHandle.h>
#include <api/font/glyph/internal/SlotBitmap.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <api/font/face/Face.h>
#include "Store.h"

namespace v3d::font::glyph {

Store::Bitmap rasterize(const Store::Key& key) {
    if (key.face == nullptr || key.mode != Mode::Coverage) {
        return Store::Bitmap();
    }
    font::internal::FaceHandle& handle = key.face->handle();
    if (!handle.vary(key.coordinates) || !handle.size(key.size)) {
        return Store::Bitmap();
    }
    // light hinting moves the outline whole pixels only vertically, so the step moves the glyph
    // a fraction of a pixel across and no hint moves it back
    FT_Vector step = { static_cast<FT_Pos>(key.subpixel % SUBPIXEL_STEPS) * static_cast<FT_Pos>(font::internal::FaceHandle::FIXED_POINT_SCALE) / SUBPIXEL_STEPS,
        0 };
    FT_Set_Transform(handle.face, nullptr, &step);
    const FT_Error error = FT_Load_Glyph(handle.face, key.index, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT);
    // the next font to load from the face sets its own size, but not always its own transform
    FT_Set_Transform(handle.face, nullptr, nullptr);
    if (error != 0) {
        return Store::Bitmap();
    }

    return internal::slotBitmap(*handle.face->glyph);
}

};  // namespace v3d::font::glyph

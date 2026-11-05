/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Field.h"

#include <api/font/internal/FaceHandle.h>
#include <api/font/internal/LibraryHandle.h>
#include <api/font/glyph/internal/SlotBitmap.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H

#include <api/font/face/Face.h>
#include <api/font/face/Library.h>
#include <api/font/text/Style.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "Store.h"

#include <glm/common.hpp>
#include <glm/vec2.hpp>

namespace v3d::font::glyph {

namespace {

/**
 * Spreads are rounded up to a multiple of this, so that effects of nearby reaches share a field.
 **/
constexpr unsigned int SPREAD_STEP = 4;

};  // namespace

Bitmap field(const Key& key) {
    if (key.face == nullptr || key.mode != Mode::Distance || key.spread == 0) {
        return Bitmap();
    }
    font::internal::FaceHandle& handle = key.face->handle();
    if (!handle.vary(key.coordinates) || !handle.size(key.size)) {
        return Bitmap();
    }
    // the spread is the library's, not the face's, so it is set for each field made
    FT_Int spread = std::min(key.spread, MAX_SPREAD);
    FT_Property_Set(key.face->library()->handle().library, "sdf", "spread", &spread);
    // the face is sized at many times its horizontal resolution, which the transform takes back
    FT_Matrix matrix = {
        static_cast<FT_Fixed>((1.0 / font::internal::FaceHandle::HORIZONTAL_OVERSAMPLE) * 0x10000L),  // checked: a constant, 1024
        0,
        0,
        static_cast<FT_Fixed>(0x10000L)
    };
    FT_Set_Transform(handle.face, &matrix, nullptr);
    if (FT_Load_Glyph(handle.face, key.index, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP) != 0 ||
        FT_Render_Glyph(handle.face->glyph, FT_RENDER_MODE_SDF) != 0) {
        return Bitmap();
    }
    return internal::slotBitmap(*handle.face->glyph);
}

std::uint8_t fieldSpread(float reach) {
    if (!std::isfinite(reach) || reach <= 0.0F) {
        return SPREAD_STEP;
    }
    const float pixels = std::ceil((reach * FIELD_SIZE) + 1.0F);
    const float stepped = std::ceil(pixels / SPREAD_STEP) * SPREAD_STEP;
    return static_cast<std::uint8_t>(std::min(stepped, static_cast<float>(MAX_SPREAD)));  // checked: from 4 to 32
}

Key fieldKey(const face::Face* face, unsigned int index, float reach, const std::vector<std::int32_t>& coordinates) {
    return Key{ .face = face, .index = index, .size = FIELD_SIZE, .mode = Mode::Distance, .spread = fieldSpread(reach), .coordinates = coordinates };
}

float effectReach(const text::Effect& effect, float size) {
    if (!(size > 0.0F)) {
        return 0.0F;
    }
    const float width = effect.width;
    return std::max(std::abs(width), width + std::max(effect.softness, 0.0F)) / size;
}

glm::vec2 effectRamp(const text::Effect& effect, float size, std::uint8_t spread) {
    if (!(size > 0.0F) || spread == 0) {
        return glm::vec2(0.5F);
    }
    // a field's value moves by half over its spread, and an em is FIELD_SIZE of its pixels
    const float perPixel = FIELD_SIZE / (size * 2.0F * static_cast<float>(spread));
    const float whole = 0.5F - (effect.width * perPixel);
    const float gone = whole - (std::max(effect.softness, 0.0F) * perPixel);
    return glm::clamp(glm::vec2(whole, gone), glm::vec2(0.0F), glm::vec2(1.0F));
}

};  // namespace v3d::font::glyph

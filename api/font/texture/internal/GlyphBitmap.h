/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

namespace v3d::font::texture::internal {

/**
 * What one rendered glyph's bitmap is, whichever of the two paths produced it.
 **/
struct GlyphBitmap final {
    FT_Bitmap bitmap = {};
    int width = 0;
    int rows = 0;
    int top = 0;
    int left = 0;
};

};  // namespace v3d::font::texture::internal

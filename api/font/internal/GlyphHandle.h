/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

namespace v3d::font::internal {

/**
 * Owns the glyph the outline path builds, until the end of the loop iteration that built it.
 *
 * It usually holds nothing: a glyph with no outline is read from the face's own slot, which
 * the face owns. Only FT_Get_Glyph returns a copy that must be released. The destructor
 * releases it however the iteration ends, whether a full atlas skips to the next glyph or a
 * failed stroke ends the load.
 **/
class GlyphHandle final {
 public:
    GlyphHandle() = default;
    ~GlyphHandle() {
        if (glyph_ != nullptr) {
            FT_Done_Glyph(glyph_);
        }
    }
    GlyphHandle(const GlyphHandle&) = delete;
    GlyphHandle& operator=(const GlyphHandle&) = delete;

    /**
     * Where FT_Get_Glyph writes. A failure before that call leaves it null. After that call,
     * it holds the glyph from the last step that succeeded.
     **/
    FT_Glyph* address() {
        return &glyph_;
    }

 private:
    FT_Glyph glyph_ = nullptr;
};

};  // namespace v3d::font::internal

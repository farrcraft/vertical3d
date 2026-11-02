/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace v3d::font::internal {

class SvgDocuments;

/**
 * Owns one FreeType face from construction to destruction, and the HarfBuzz face that shapes
 * text over it. face and shaping are null when the file could not be opened or has no Unicode
 * character map.
 **/
class FaceHandle final {
 public:
    /**
     * @param library the library the face is opened through, which has to outlive it
     **/
    FaceHandle(FT_Library library, const char* filename);
    ~FaceHandle();
    FaceHandle(const FaceHandle&) = delete;
    FaceHandle& operator=(const FaceHandle&) = delete;

    /**
     * Set the size glyphs are loaded at, in pixels. A face has one size at a time, so a font
     * sets its size before each load that may follow another font's.
     *
     * The face is loaded at HORIZONTAL_OVERSAMPLE times its horizontal resolution, and the
     * transform scales x back down by the same factor. Hinting then fits outlines to whole
     * pixels vertically and to a fraction of a pixel horizontally.
     *
     * @return whether FreeType took the size
     **/
    bool size(float pixels);

    /**
     * Set the normalized coordinates glyphs are loaded at, in 2.14 fixed point, or none for
     * every axis's default. A face has one set at a time, so every load sets those of its own
     * glyph first. Setting the coordinates already set costs nothing.
     *
     * @return whether FreeType took them. A face with no axes takes only none
     **/
    bool vary(const std::vector<std::int32_t>& coordinates);

    // metrics and advances are 26.6 fixed point, 64 units to the pixel
    static constexpr float FIXED_POINT_SCALE = 64.0F;
    static constexpr FT_UInt HORIZONTAL_OVERSAMPLE = 64;

    FT_Face face = nullptr;
    hb_face_t* shaping = nullptr;
    std::unique_ptr<SvgDocuments> documents;  /**< the face's SVG documents, as they are parsed **/
    std::vector<std::int32_t> varied;         /**< the coordinates FreeType holds, set only by vary() **/

 private:

    // the resolution FT_Set_Char_Size is given, in dots per inch, at which one point is one pixel
    static constexpr FT_UInt RESOLUTION_DPI = 72;
};

};  // namespace v3d::font::internal

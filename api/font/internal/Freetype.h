/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::font::internal {

/**
 * The FreeType library and face a font is read through, open only while it is being read.
 * release() closes whatever is open, and so do loadFace() and the destructor. A load that
 * fails part way therefore never has to track which of the two it opened.
 **/
class Freetype final {
 public:
    explicit Freetype(const boost::shared_ptr<v3d::log::Logger> & logger);
    ~Freetype();

    Freetype(const Freetype&) = delete;
    Freetype& operator=(const Freetype&) = delete;

    bool loadFace(const std::string& filename, float size);

    void release();

    // metrics and advances are 26.6 fixed point, 64 units to the pixel
    static constexpr float FIXED_POINT_SCALE = 64.0F;
    // the face is loaded at this many times its horizontal resolution, and the transform matrix
    // scales x back down by the same factor
    static constexpr FT_UInt HORIZONTAL_OVERSAMPLE = 64;

    FT_Library library_ = nullptr;
    FT_Face face_ = nullptr;
    boost::shared_ptr<v3d::log::Logger> logger_;

 private:
    // the resolution FT_Set_Char_Size is given, in dots per inch, at which one point is one pixel
    static constexpr FT_UInt RESOLUTION_DPI = 72;
};

};  // namespace v3d::font::internal

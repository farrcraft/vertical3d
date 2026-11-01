/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

namespace v3d::font::internal {

/**
 * Owns one FreeType library from construction to destruction. library is null when FreeType
 * could not be initialized.
 **/
class LibraryHandle final {
 public:
    LibraryHandle();
    ~LibraryHandle();
    LibraryHandle(const LibraryHandle&) = delete;
    LibraryHandle& operator=(const LibraryHandle&) = delete;

    FT_Library library = nullptr;
};

};  // namespace v3d::font::internal

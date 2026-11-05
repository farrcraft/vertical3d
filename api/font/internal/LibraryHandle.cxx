/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "LibraryHandle.h"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace v3d::font::internal {

LibraryHandle::LibraryHandle() {
    if (FT_Init_FreeType(&library) != 0) {
        library = nullptr;
    }
}

LibraryHandle::~LibraryHandle() {
    if (library != nullptr) {
        FT_Done_FreeType(library);
    }
}

};  // namespace v3d::font::internal

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "FaceHandle.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb-ft.h>
#include <hb.h>

namespace v3d::font::internal {

FaceHandle::FaceHandle(FT_Library library, const char* filename) {
    if (library == nullptr || FT_New_Face(library, filename, 0, &face) != 0) {
        face = nullptr;
        return;
    }
    if (FT_Select_Charmap(face, FT_ENCODING_UNICODE) != 0) {
        FT_Done_Face(face);
        face = nullptr;
        return;
    }
    // the HarfBuzz face takes a reference of its own on the FreeType face
    shaping = hb_ft_face_create_referenced(face);
}

FaceHandle::~FaceHandle() {
    if (shaping != nullptr) {
        hb_face_destroy(shaping);
    }
    if (face != nullptr) {
        FT_Done_Face(face);
    }
}

bool FaceHandle::size(float pixels) {
    if (face == nullptr) {
        return false;
    }
    if (FT_Set_Char_Size(face, static_cast<FT_F26Dot6>(pixels * FIXED_POINT_SCALE), 0, RESOLUTION_DPI * HORIZONTAL_OVERSAMPLE, RESOLUTION_DPI) != 0) {
        return false;
    }
    FT_Matrix matrix = {
        static_cast<FT_Fixed>((1.0 / HORIZONTAL_OVERSAMPLE) * 0x10000L),  // checked: a constant, 1024
        0,
        0,
        static_cast<FT_Fixed>(0x10000L)
    };
    FT_Set_Transform(face, &matrix, nullptr);
    return true;
}

};  // namespace v3d::font::internal

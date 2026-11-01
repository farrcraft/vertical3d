/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Freetype.h"

#include <api/log/Logger.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstddef>
#include <string>

namespace v3d::font::internal {

Freetype::Freetype(const boost::shared_ptr<v3d::log::Logger>& logger) : logger_(logger) {
}

Freetype::~Freetype() {
    release();
}

void Freetype::release() {
    if (face_ != nullptr) {
        FT_Done_Face(face_);
        face_ = nullptr;
    }
    if (library_ != nullptr) {
        FT_Done_FreeType(library_);
        library_ = nullptr;
    }
}

bool Freetype::loadFace(const std::string& filename, float size) {
    release();

    // initialize freetype library
    FT_Error error;
    error = FT_Init_FreeType(&library_);
    if (error != 0) {
        logger_->get()->error("Error initializing freetype library!");
        return false;
    }

    // load font file
    error = FT_New_Face(library_, filename.c_str(), 0, &face_);
    if (error != 0) {
        logger_->get()->error("Error creating new freetype face!");
        release();
        return false;
    }

    // select charmap
    error = FT_Select_Charmap(face_, FT_ENCODING_UNICODE);
    if (error != 0) {
        logger_->get()->error("Error selecting freetype charmap!");
        release();
        return false;
    }

    // set char size
    // size *= 100.0f;
    error = FT_Set_Char_Size(face_, static_cast<int>((size * FIXED_POINT_SCALE)), 0, RESOLUTION_DPI * HORIZONTAL_OVERSAMPLE, RESOLUTION_DPI);
    if (error != 0) {
        logger_->get()->error("Error setting freetype char size!");
        release();
        return false;
    }

    // set transform matrix
    FT_Matrix matrix = {
        static_cast<int>((1.0 / HORIZONTAL_OVERSAMPLE) * 0x10000L),  // checked: a constant, 1024
        static_cast<int>(0.0 * 0x10000L),  // checked: a constant, 0
        static_cast<int>(0.0 * 0x10000L),  // checked: a constant, 0
        static_cast<int>(1.0 * 0x10000L)  // checked: a constant, 65536
    };
    FT_Set_Transform(face_, &matrix, NULL);

    return true;
}

};  // namespace v3d::font::internal

/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <png.h>

namespace v3d::image::writer::internal {

/**
 * The picture in the shape libpng's header takes.
 **/
struct Picture {
    png_uint_32 width = 0;
    png_uint_32 height = 0;
    int depth = 8;
    int colour = PNG_COLOR_TYPE_RGB;
    png_color_8 significant = {};
    png_bytepp rows = nullptr;
};

};  // namespace v3d::image::writer::internal

/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>

namespace v3d::image::reader::internal {

/**
 * How far libpng has read into the buffer. libpng reads in order and never seeks.
 **/
struct Cursor final {
    const unsigned char* data;
    std::size_t size;
    std::size_t at;
};

};  // namespace v3d::image::reader::internal

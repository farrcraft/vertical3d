/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>

namespace v3d::image {

/**
 * Copy pixels with the first and third bytes of each exchanged: RGB to BGR and back. BGR is
 * the order BMP and TGA keep on disk. Any bytes after the third, an alpha, are copied as they
 * are. The two buffers may be one, for a swap in place.
 *
 * The run is how many pixels there are, and how many bytes each takes, at least three.
 **/
struct Run {
    std::size_t pixels = 0;
    unsigned int channels = 3;
};

void swapRedBlue(const unsigned char* from, unsigned char* to, const Run& run);

};  // namespace v3d::image

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>

namespace v3d::image {

/**
 * Copy pixels with the first and third bytes of each exchanged - RGB to BGR and back, which is
 * the order BMP and TGA keep on disk - and any bytes after the third, an alpha, as they are.
 * The two buffers may be one, for a swap in place.
 *
 * @param channels bytes a pixel, at least three
 **/
void swapRedBlue(const unsigned char* from, unsigned char* to, std::size_t pixels, unsigned int channels);

};  // namespace v3d::image

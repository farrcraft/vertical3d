/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya {

/**
 * The pixels a raster space bound touches, clipped to a frame of columns by rows: left, top,
 * right and bottom, each inclusive. Only x and y of the bound are read.
 *
 * A bound with a NaN in it touches no pixel, and neither does a frame with no pixels. No pixel
 * is a right edge left of the left one, or a bottom edge above the top one. A bound far off the
 * frame or infinite is held within one pixel of the frame before it is made an integer, so it
 * touches no pixel or a whole row or column of them.
 */
std::array<int, 4> footprint(const glm::vec3 & min, const glm::vec3 & max, int columns, int rows);

/**
 * Which of a number of equal slices of the shutter a time falls in, from 0 to slices - 1.
 *
 * A time before the shutter opens, -infinity and a NaN take the first slice. A time after the
 * shutter closes and +infinity take the last. Every time takes the first slice when the shutter
 * does not open, when either end of it is a NaN or infinite, and when slices is 0.
 */
unsigned int shutterSlice(float time, const glm::vec2 & shutter, unsigned int slices);

};  // namespace v3d::moya

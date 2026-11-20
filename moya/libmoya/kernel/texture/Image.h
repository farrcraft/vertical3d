/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::kernel::texture {

/**
 * What a program's image node reads: a grid of textures, a column for each unit of u and a row
 * for each unit of v. An image of one file is a grid of one, read whatever the coordinate. A UDIM
 * image is ten columns wide, and a missing tile names kNoTexture.
 *
 * - first is the place of the grid's first texture among the catalog's tiles, row after row from
 *   v's zero.
 **/
struct Image {
    std::uint32_t first { 0 };
    std::uint32_t columns { 1 };
    std::uint32_t rows { 1 };
};

};  // namespace v3d::moya::kernel::texture

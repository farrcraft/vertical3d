/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/texture/internal/TileKey.h>

#include <cstdint>
#include <list>
#include <vector>

namespace v3d::moya::texture::internal {

/**
 * A tile the cache holds: its texels' four channels each, row after row from its top, its width,
 * and its place in the cache's order of use.
 **/
struct Tile {
    std::vector<float> texels;
    std::uint32_t width { 0 };
    std::list<TileKey>::iterator used;
};

};  // namespace v3d::moya::texture::internal

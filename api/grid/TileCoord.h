/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::grid {

/**
 * Integer address of a tile on the grid.
 *
 * x runs along world +X and y along world +Z: the grid lies flat, so the second axis is
 * depth, not height. Out of range values are allowed, because worldToTile() answers where a
 * point would be and the caller asks TileGrid::contains() whether that is on the map.
 **/
struct TileCoord {
    int x{0};
    int y{0};

    friend bool operator==(TileCoord, TileCoord) = default;
};

};  // namespace v3d::grid

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::grid {

/**
 * Integer address of a tile on the grid.
 *
 * x runs along world +X and y along world +Z. The grid lies flat, so the second axis is depth,
 * not height. A coordinate may lie off the grid. worldToTile() returns one for a point off the
 * map, and the caller checks TileGrid::contains().
 **/
struct TileCoord {
    int x{0};
    int y{0};

    friend bool operator==(TileCoord, TileCoord) = default;
};

};  // namespace v3d::grid

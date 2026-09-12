/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#include "Sight.h"

#include <api/grid/LineOfSight.h>

#include <algorithm>

namespace odyssey::tile {

/**
 **/
void Sight::look(const v3d::grid::TileGrid& grid, v3d::grid::TileCoord from) {
    if (width_ != grid.width() || height_ != grid.height()) {
        // a board of another size is another board, so nothing about the old one is
        // remembered onto it
        width_ = grid.width();
        height_ = grid.height();
        const std::size_t tiles = static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
        visible_.assign(tiles, 0);
        remembered_.assign(tiles, 0);
    }

    std::ranges::fill(visible_, 0);
    // only the range box is traced: a tile outside it is out of sight whatever the board
    // does, and the box is the Chebyshev disc rather than an approximation of one
    const int minX = std::max(from.x - RANGE, 0);
    const int minY = std::max(from.y - RANGE, 0);
    const int maxX = std::min(from.x + RANGE, width_ - 1);
    const int maxY = std::min(from.y + RANGE, height_ - 1);
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            const v3d::grid::TileCoord tile{x, y};
            if (!v3d::grid::hasLineOfSight(grid, from, tile)) {
                continue;
            }
            const std::size_t at = index(tile);
            visible_[at] = 1;
            remembered_[at] = 1;
        }
    }
}

/**
 **/
bool Sight::visible(v3d::grid::TileCoord tile) const {
    const std::size_t at = index(tile);
    return at < visible_.size() && visible_[at] != 0;
}

/**
 **/
bool Sight::remembered(v3d::grid::TileCoord tile) const {
    const std::size_t at = index(tile);
    return at < remembered_.size() && remembered_[at] != 0;
}

/**
 **/
std::size_t Sight::index(v3d::grid::TileCoord tile) const {
    if (tile.x < 0 || tile.y < 0 || tile.x >= width_ || tile.y >= height_) {
        return visible_.size();
    }
    return static_cast<std::size_t>(tile.y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(tile.x);
}

};  // namespace odyssey::tile

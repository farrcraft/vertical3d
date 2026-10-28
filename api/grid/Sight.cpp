/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Sight.h"

#include <algorithm>
#include <cstddef>
#include <optional>

#include "GridSet.h"
#include "LineOfSight.h"
#include "TileCoord.h"
#include "TileGrid.h"

namespace v3d::grid {

Sight::Sight(int range) : range_(range) {
}

int Sight::range() const noexcept {
    return range_;
}

void Sight::look(const TileGrid& grid, TileCoord from, const SightBlocker& blocks) {
    look(GridSet::local(grid), grid, from, blocks);
}

void Sight::look(const GridSet& grids, const TileGrid& board, TileCoord from, const SightBlocker& blocks) {
    if (width_ != board.width() || height_ != board.height()) {
        // a board of another size is a different board, so its memory starts empty
        width_ = board.width();
        height_ = board.height();
        const std::size_t tiles = static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
        visible_.assign(tiles, 0);
        remembered_.assign(tiles, 0);
    }

    std::ranges::fill(visible_, 0);
    const std::optional<TileCoord> origin = grids.origin(board);
    if (!origin) {
        return;
    }
    // trace only the board's tiles within range. A tile outside the square is out of sight, and
    // the square is exactly the set of tiles within range Chebyshev steps
    const int minX = std::max(from.x - range_, origin->x);
    const int minY = std::max(from.y - range_, origin->y);
    const int maxX = std::min(from.x + range_, origin->x + width_ - 1);
    const int maxY = std::min(from.y + range_, origin->y + height_ - 1);
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            if (!hasLineOfSight(grids, from, TileCoord{x, y}, blocks)) {
                continue;
            }
            const std::size_t at = index(TileCoord{x - origin->x, y - origin->y});
            visible_[at] = 1;
            remembered_[at] = 1;
        }
    }
}

void Sight::blind() {
    std::ranges::fill(visible_, 0);
}

bool Sight::visible(TileCoord tile) const {
    const std::size_t at = index(tile);
    return at < visible_.size() && visible_[at] != 0;
}

bool Sight::remembered(TileCoord tile) const {
    const std::size_t at = index(tile);
    return at < remembered_.size() && remembered_[at] != 0;
}

std::size_t Sight::index(TileCoord tile) const {
    if (tile.x < 0 || tile.y < 0 || tile.x >= width_ || tile.y >= height_) {
        return visible_.size();
    }
    return (static_cast<std::size_t>(tile.y) * static_cast<std::size_t>(width_)) + static_cast<std::size_t>(tile.x);
}

};  // namespace v3d::grid

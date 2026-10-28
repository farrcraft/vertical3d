/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "GridSet.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#include <glm/vec3.hpp>

#include "TileCoord.h"
#include "TileGrid.h"

namespace v3d::grid {

namespace {

/**
 * How far a corner may sit from the tile lattice, as a fraction of a tile, and still be on it.
 * A corner computed in floats lands a rounding error away from a whole number of tiles.
 **/
constexpr float LATTICE_TOLERANCE = 1.0e-3F;

/**
 * How far apart two tile sizes may be, as a fraction of the larger, and still be the same. A size
 * read from a file carries a rounding error.
 **/
constexpr float SIZE_TOLERANCE = 1.0e-6F;

bool sameSize(float first, float second) {
    return std::abs(first - second) <= SIZE_TOLERANCE * std::max(std::abs(first), std::abs(second));
}

};  // namespace

GridSet GridSet::local(const TileGrid& grid) {
    GridSet set;
    set.tileSize_ = grid.tileSize();
    set.place(grid, TileCoord{0, 0});
    return set;
}

void GridSet::add(const TileGrid& grid) {
    const std::optional<std::string> refused = refusal(grid);
    if (refused) {
        throw std::invalid_argument(*refused);
    }
    if (grids_.empty()) {
        tileSize_ = grid.tileSize();
    }
    place(grid, *lattice(grid));
}

std::optional<std::string> GridSet::refusal(const TileGrid& grid) const {
    if (!grids_.empty() && !sameSize(grid.tileSize(), tileSize_)) {
        std::ostringstream reason;
        reason << "a grid of tiles " << grid.tileSize() << " across cannot join a set of tiles " << tileSize_ << " across";
        return reason.str();
    }
    const std::optional<TileCoord> origin = lattice(grid);
    if (!origin) {
        std::ostringstream reason;
        reason << "a grid whose corner is at (" << grid.worldMin().x << ", " << grid.worldMin().z
            << ") is not a whole number of tiles from the world's origin";
        return reason.str();
    }
    for (const Placed& placed : grids_) {
        const bool apartX = origin->x + grid.width() <= placed.origin.x || placed.origin.x + placed.width <= origin->x;
        const bool apartY = origin->y + grid.height() <= placed.origin.y || placed.origin.y + placed.height <= origin->y;
        if (!apartX && !apartY) {
            std::ostringstream reason;
            reason << "a grid at world tile (" << origin->x << ", " << origin->y << ") overlaps one at ("
                << placed.origin.x << ", " << placed.origin.y << ")";
            return reason.str();
        }
    }
    return std::nullopt;
}

bool GridSet::empty() const noexcept {
    return grids_.empty();
}

float GridSet::tileSize() const noexcept {
    return tileSize_;
}

std::optional<TileCoord> GridSet::origin(const TileGrid& grid) const {
    for (const Placed& placed : grids_) {
        if (placed.grid == &grid) {
            return placed.origin;
        }
    }
    return std::nullopt;
}

const TileGrid* GridSet::gridAt(TileCoord tile) const {
    const Placed* placed = find(tile);
    return placed != nullptr ? placed->grid : nullptr;
}

bool GridSet::contains(TileCoord tile) const {
    return find(tile) != nullptr;
}

bool GridSet::passable(TileCoord tile) const {
    const Placed* placed = find(tile);
    return placed != nullptr && placed->grid->passable(TileCoord{tile.x - placed->origin.x, tile.y - placed->origin.y});
}

bool GridSet::opaque(TileCoord tile) const {
    const Placed* placed = find(tile);
    return placed == nullptr || placed->grid->cover(TileCoord{tile.x - placed->origin.x, tile.y - placed->origin.y}) == Cover::Full;
}

std::size_t GridSet::count() const noexcept {
    return count_;
}

std::size_t GridSet::index(TileCoord tile) const {
    const Placed* placed = find(tile);
    if (placed == nullptr) {
        return count_;
    }
    const std::size_t x = static_cast<std::size_t>(tile.x - placed->origin.x);
    const std::size_t y = static_cast<std::size_t>(tile.y - placed->origin.y);
    return placed->offset + (y * static_cast<std::size_t>(placed->width)) + x;
}

TileCoord GridSet::tile(std::size_t index) const {
    for (const Placed& placed : grids_) {
        const std::size_t tiles = static_cast<std::size_t>(placed.width) * static_cast<std::size_t>(placed.height);
        if (index >= placed.offset && index < placed.offset + tiles) {
            const std::size_t within = index - placed.offset;
            const std::size_t width = static_cast<std::size_t>(placed.width);
            return TileCoord{placed.origin.x + static_cast<int>(within % width), placed.origin.y + static_cast<int>(within / width)};
        }
    }
    return TileCoord{0, 0};
}

const GridSet::Placed* GridSet::find(TileCoord tile) const {
    for (const Placed& placed : grids_) {
        if (tile.x >= placed.origin.x && tile.x < placed.origin.x + placed.width &&
            tile.y >= placed.origin.y && tile.y < placed.origin.y + placed.height) {
            return &placed;
        }
    }
    return nullptr;
}

void GridSet::place(const TileGrid& grid, TileCoord origin) {
    grids_.push_back(Placed{.grid = &grid, .origin = origin, .width = grid.width(), .height = grid.height(), .offset = count_});
    count_ += static_cast<std::size_t>(grid.width()) * static_cast<std::size_t>(grid.height());
}

std::optional<TileCoord> GridSet::lattice(const TileGrid& grid) {
    const glm::vec3 corner = grid.worldMin();
    const float x = corner.x / grid.tileSize();
    const float y = corner.z / grid.tileSize();
    const float roundedX = std::round(x);
    const float roundedY = std::round(y);
    if (std::abs(x - roundedX) > LATTICE_TOLERANCE || std::abs(y - roundedY) > LATTICE_TOLERANCE) {
        return std::nullopt;
    }
    return TileCoord{static_cast<int>(roundedX), static_cast<int>(roundedY)};
}

};  // namespace v3d::grid

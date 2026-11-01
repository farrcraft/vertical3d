/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/grid/GridSet.h>
#include <api/grid/TileCoord.h>
#include <api/grid/TileFilter.h>

#include <cstddef>

namespace v3d::grid::internal {

/**
 * The grids plus the caller's filter, and the movement rules that read both.
 **/
class Walk {
 public:
    Walk(const GridSet& grids, const TileFilter& enterable) :
        grids_(grids),
        enterable_(enterable) {
    }

    std::size_t count() const {
        return grids_.count();
    }

    std::size_t index(TileCoord tile) const {
        return grids_.index(tile);
    }

    TileCoord coord(std::size_t index) const {
        return grids_.tile(index);
    }

    /**
     * Whether a tile may be entered at all. A tile in no grid is impassable, so this needs no
     * separate bounds test.
     **/
    bool open(TileCoord tile) const {
        return grids_.passable(tile) && (!enterable_ || enterable_(tile));
    }

    /**
     * Whether a step may be taken from a tile.
     *
     * A diagonal additionally requires one of the two tiles it passes between to be open,
     * so a wall laid corner to corner cannot be squeezed through. One blocked corner still
     * allows the step, so a mover can round the end of a wall.
     **/
    bool allowed(TileCoord from, TileCoord step) const {
        if (!open(TileCoord{ from.x + step.x, from.y + step.y })) {
            return false;
        }
        if (step.x == 0 || step.y == 0) {
            return true;
        }
        return open(TileCoord{ from.x + step.x, from.y }) || open(TileCoord{ from.x, from.y + step.y });
    }

 private:
    const GridSet& grids_;
    const TileFilter& enterable_;
};

};  // namespace v3d::grid::internal

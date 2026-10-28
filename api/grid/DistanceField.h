/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "GridSet.h"
#include "TileCoord.h"
#include "TileFilter.h"
#include "TileGrid.h"

#include <limits>
#include <vector>

namespace v3d::grid {

/**
 * What a route to one goal tile costs from every tile of a grid at once.
 *
 * The constructor floods outward from the goal, and cost() returns the cost of a route to the
 * goal from any tile of the grid. Use it, not tileDistance(), to move toward a goal.
 * tileDistance() ignores what is in the way. For a goal behind a wall, the tile it calls
 * closest is the tile against the wall, and a mover there has no closer tile to go to.
 *
 * The goal is never tested against the grid or the filter, as findPath() never tests its
 * start tile. Whoever stands on the goal does not make it unreachable, and a flood that starts
 * on an occupied tile can still leave it. Every other tile in the field passed the filter. A
 * step costs the same in both directions, and a diagonal step passes between the same two
 * tiles from either end. The cost recorded on a tile is therefore the cost of a route from that
 * tile to the goal.
 *
 * The constructor floods the whole grid. Build one field outside any loop over candidate
 * tiles, not one per tile.
 **/
class DistanceField {
 public:
    /**
     * The cost of a tile with no route to the goal. cost() also returns it for a tile off the
     * grid.
     **/
    static constexpr int UNREACHABLE = std::numeric_limits<int>::max();

    /**
     * Flood the board outward from goal.
     **/
    DistanceField(const TileGrid& grid, TileCoord goal, const TileFilter& enterable = {});

    /**
     * Flood a set of grids outward from goal, in world tiles. The field keeps where the grids
     * lie but reads none of them after it is built, so it may outlive them.
     **/
    DistanceField(const GridSet& grids, TileCoord goal, const TileFilter& enterable = {});

    /**
     * What the cheapest route from tile to the goal costs.
     *
     * @return UNREACHABLE for a tile off the grids and for one no route reaches
     **/
    int cost(TileCoord tile) const;

    /**
     * @return whether any route from tile reaches the goal
     **/
    bool reaches(TileCoord tile) const;

 private:
    /** Where the grids lie, which cost() reads to find a tile's entry. It reads no grid. **/
    GridSet grids_;
    std::vector<int> costs_;
};

};  // namespace v3d::grid

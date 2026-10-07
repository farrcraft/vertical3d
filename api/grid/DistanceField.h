/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "TileCoord.h"
#include "TileFilter.h"
#include "TileGrid.h"

#include <limits>
#include <vector>

namespace v3d::grid {

/**
 * What a route to one goal tile costs from every tile of a grid at once.
 *
 * A flood fill outward from the goal, so cost() returns the cost of a route to the goal from
 * any tile of the board. Use it rather than tileDistance() to move toward a goal.
 * tileDistance() ignores what is in the way. The tile it calls closest to a goal behind a
 * wall is the tile against the wall, and a mover there has no closer tile to go to.
 *
 * The goal is never tested against the grid or the filter, exactly as findPath() never
 * tests the tile its mover is standing on. Whoever is standing on the goal does not make it
 * unreachable. Without the exemption, a flood seeded on an occupied tile could not leave it.
 * Every other tile of the field was admitted by the filter. Both the step cost and the corner
 * rule read the same in either direction. The cost recorded on a tile is therefore what a
 * route from that tile to the goal costs.
 *
 * Building one floods the whole board, so build it once outside a loop over candidate tiles
 * rather than once per tile.
 **/
class DistanceField {
 public:
    /**
     * What a tile with no route to the goal costs, and what cost() returns for an off grid
     * tile.
     **/
    static constexpr int UNREACHABLE = std::numeric_limits<int>::max();

    /**
     * Flood the board outward from goal.
     **/
    DistanceField(const TileGrid& grid, TileCoord goal, const TileFilter& enterable = {});

    /**
     * What the cheapest route from tile to the goal costs.
     *
     * @return UNREACHABLE for a tile off the grid and for one no route reaches
     **/
    int cost(TileCoord tile) const;

    /**
     * @return whether any route from tile reaches the goal
     **/
    bool reaches(TileCoord tile) const;

 private:
    int width_{0};
    std::vector<int> costs_;
};

};  // namespace v3d::grid

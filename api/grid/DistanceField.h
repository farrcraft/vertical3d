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
 * A flood fill outward from the goal, so cost() answers "what would getting from here to
 * there cost" for the whole board. That is the difference between closing on something and
 * walking into the wall in front of it: tileDistance() ignores what is in the way, so the
 * tile it calls closest to a goal on the far side of a wall is the tile against the wall,
 * and whatever moved there has nowhere left to go that is closer.
 *
 * The goal is never tested against the grid or the filter, exactly as findPath() never
 * tests the tile its mover is standing on: whoever is standing on the goal does not make it
 * unreachable, and without the exemption a flood seeded on an occupied tile could not leave
 * it. Every other tile of the field was admitted by the filter, and both the step cost and
 * the corner rule read the same in either direction, so the cost recorded on a tile is what
 * a route from that tile to the goal costs.
 *
 * Building one floods the whole board, so it is worth hoisting out of a loop over candidate
 * tiles rather than asked once per tile.
 **/
class DistanceField {
 public:
    /**
     * What a tile with no route to the goal costs, and what an off grid tile answers.
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

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "TileGrid.h"

#include <functional>
#include <limits>
#include <vector>

namespace v3d::grid {

/**
 * Whether a tile may be entered, over and above the grid's own passability.
 *
 * Supplied by the caller so that nothing here learns what a mover is: an app passes a
 * predicate closed over whatever it tracks - occupancy, ownership, a zone it will not cross
 * - and the grid stays the same type for all of them. A default constructed filter imposes
 * no restriction, leaving passability to the grid alone.
 **/
typedef std::function<bool(TileCoord)> TileFilter;

/**
 * A tile a flood fill reached, and what reaching it cost.
 **/
struct ReachableTile {
    TileCoord tile;
    int cost{0};
};

/**
 * Cost of one orthogonal step between adjacent tiles.
 *
 * Movement is 8 way and a diagonal costs the same as an orthogonal step, so distance on
 * open ground is Chebyshev: a budget of N reaches a square of side 2N + 1, and eight movers
 * can stand in contact with one. The price is that a diagonal covers 41% more ground for
 * the same cost, so a diagonal approach is always the cheapest one.
 *
 * These are two constants rather than one because the search charges per edge. Pricing
 * diagonals differently is a change to these values and to tileDistance(), not to the
 * algorithm.
 **/
constexpr int ORTHOGONAL_STEP_COST = 1;

/**
 * Cost of one diagonal step, which is the orthogonal cost - see ORTHOGONAL_STEP_COST.
 **/
constexpr int DIAGONAL_STEP_COST = 1;

/**
 * Steps between two tiles ignoring everything in the way: Chebyshev distance, which is the
 * admissible heuristic for the costs above.
 *
 * This is the distance metric for anything measured in tiles. A second metric invented
 * elsewhere would disagree with what movement charges.
 **/
int tileDistance(TileCoord a, TileCoord b);

/**
 * Cheapest route from start to goal, start first and goal last.
 *
 * The returned path always includes the tile the mover is standing on, so its cost is
 * path.size() - 1 and a route to where you already stand is one tile long. An empty result
 * means there is no route at all, which is why the start is included: it keeps "nowhere to
 * go" distinct from "already there".
 *
 * start itself is never tested against the grid or the filter - the mover is standing on
 * it, and whatever makes a tile unenterable does not trap whoever is already there.
 *
 * A diagonal step between two blocked tiles is refused: a wall laid corner to corner is a
 * wall, and two movers standing corner to corner cannot be slipped between. Cutting a
 * single blocked corner is allowed, because rounding the end of a wall passes through
 * nothing.
 *
 * Ties are broken by a fixed neighbour order, so the same query on the same map always
 * returns the same path.
 **/
std::vector<TileCoord> findPath(const TileGrid& grid, TileCoord start, TileCoord goal,
    const TileFilter& enterable = {});

/**
 * Every tile reachable from start for no more than budget, cheapest first.
 *
 * Includes start at a cost of 0. The same corner rule and the same treatment of the start
 * tile as findPath(), so a tile in this set is a tile findPath() can reach for the cost
 * given here - a highlight drawn from this cannot promise a move the path search then
 * refuses.
 *
 * @throws std::invalid_argument if the budget is negative
 **/
std::vector<ReachableTile> reachableTiles(const TileGrid& grid, TileCoord start, int budget,
    const TileFilter& enterable = {});

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

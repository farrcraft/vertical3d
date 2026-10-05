/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "ReachableTile.h"
#include "TileCoord.h"
#include "TileFilter.h"
#include "TileGrid.h"

#include <vector>

namespace v3d::grid {

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
 * This is the distance metric for anything measured in tiles. Use it rather than another
 * metric, so that distances agree with what movement costs.
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
 * A diagonal step between two blocked tiles is refused, so a wall laid corner to corner
 * blocks movement, and a mover cannot slip between two movers standing corner to corner.
 * Cutting a single blocked corner is allowed, so a mover can round the end of a wall.
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
 * given here. A highlight drawn from this set never shows a move that findPath() refuses.
 *
 * @throws std::invalid_argument if the budget is negative
 **/
std::vector<ReachableTile> reachableTiles(const TileGrid& grid, TileCoord start, int budget,
    const TileFilter& enterable = {});

};  // namespace v3d::grid

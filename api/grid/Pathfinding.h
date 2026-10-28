/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "GridSet.h"
#include "ReachableTile.h"
#include "TileCoord.h"
#include "TileFilter.h"
#include "TileGrid.h"

#include <vector>

namespace v3d::grid {

/**
 * Cost of one orthogonal step between adjacent tiles.
 *
 * Movement is 8-way, and a diagonal step costs the same as an orthogonal one. Distance on open
 * ground is therefore Chebyshev distance. A budget of N reaches a square of side 2N + 1, and
 * eight movers can stand next to one. A diagonal step covers 41% more ground for the same
 * cost, so a diagonal approach is never dearer than another.
 *
 * The search charges each step by its direction, so there are two constants. Pricing
 * diagonals differently changes these values and tileDistance(), not the search.
 **/
constexpr int ORTHOGONAL_STEP_COST = 1;

/**
 * Cost of one diagonal step. It equals ORTHOGONAL_STEP_COST, whose comment explains why.
 **/
constexpr int DIAGONAL_STEP_COST = 1;

/**
 * The number of steps between two tiles, ignoring everything in the way. This is Chebyshev
 * distance, the admissible heuristic for the costs above.
 *
 * Measure every distance in tiles with it, so that distances agree with movement costs.
 **/
int tileDistance(TileCoord from, TileCoord to);

/**
 * Cheapest route from start to goal, start first and goal last.
 *
 * The path always includes start, so its cost is path.size() - 1, and a route from a tile to
 * itself is one tile long. An empty result means there is no route. Including start keeps
 * "no route" distinct from "already there".
 *
 * start is never tested against the grid or the filter. The mover is standing on it, so
 * whatever makes a tile unenterable does not trap a mover already there.
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
 * Includes start at a cost of 0. It uses the same corner rule and the same treatment of start
 * as findPath(), so findPath() reaches each tile in the set for the cost given here. A
 * highlight drawn from this set never shows a move that findPath() refuses.
 *
 * @throws std::invalid_argument if the budget is negative
 **/
std::vector<ReachableTile> reachableTiles(const TileGrid& grid, TileCoord start, int budget,
    const TileFilter& enterable = {});

/**
 * The cheapest route across a set of grids, in world tiles, by the same rules as findPath() on
 * one grid. A tile in no grid of the set is impassable, so a route never leaves the grids, and
 * the filter is asked about world tiles.
 **/
std::vector<TileCoord> findPath(const GridSet& grids, TileCoord start, TileCoord goal,
    const TileFilter& enterable = {});

/**
 * Every tile of a set of grids reachable from start for no more than budget, in world tiles, by
 * the same rules as reachableTiles() on one grid.
 *
 * @throws std::invalid_argument if the budget is negative
 **/
std::vector<ReachableTile> reachableTiles(const GridSet& grids, TileCoord start, int budget,
    const TileFilter& enterable = {});

};  // namespace v3d::grid

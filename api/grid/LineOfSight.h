/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "TileCoord.h"
#include "TileGrid.h"

#include <functional>
#include <vector>

namespace v3d::grid {

/**
 * Whether a tile stops sight passing through it, over and above the grid's own cover.
 *
 * Supplied by the caller, as TileFilter is for movement, so that this library has no notion
 * of an occupant. Smoke or a closed shutter is an extra blocker the map does not carry. A
 * default constructed blocker adds nothing, leaving sight to the grid alone.
 *
 * A blocker whose result depends on which endpoint is looking breaks the symmetry guarantee
 * below. For that reason it receives only the tile.
 **/
typedef std::function<bool(TileCoord)> SightBlocker;

/**
 * Whether nothing blocks sight between two tiles.
 *
 * The relation is symmetric: hasLineOfSight(a, b) and hasLineOfSight(b, a) are the same
 * call. The two endpoints are put in a fixed order before anything is traced, so one line is
 * tested for both directions and there is no tie for the direction of travel to break
 * differently.
 *
 * Only Cover::Full on the grid blocks, and the two endpoints are never tested - an occupant
 * standing in cover can see out of it, the same way findPath() never tests the tile the
 * mover is standing on. Sight between a tile and itself, or between neighbours, is therefore
 * always clear.
 *
 * Sight squeezes through a corner exactly as movement does: where the line passes precisely
 * through the point four tiles share, it is stopped only if both tiles it passes between
 * block. A line that passes the end of a wall is therefore not stopped by it.
 *
 * Off grid endpoints have no sight, so this needs no separate bounds test.
 **/
bool hasLineOfSight(const TileGrid& grid, TileCoord from, TileCoord to, const SightBlocker& blocks = {});

/**
 * The tiles the sight line passes through, from first and to last.
 *
 * Every tile the segment between the two centres touches, so consecutive entries are one 8
 * way step apart. The sequence is the path a shot travels along, for an overlay to draw or
 * for a rule that applies to each tile passed through. Empty when either endpoint is off the
 * grid; one tile long when they are the same.
 *
 * Reversing the arguments reverses the result exactly, because the same trace serves both
 * directions.
 *
 * Nothing here says whether sight is clear. Where the line squeezes through a corner the two
 * tiles it passes between are absent from this sequence, so testing these tiles for
 * blockers gives a different result from hasLineOfSight(). Call that instead.
 **/
std::vector<TileCoord> sightLine(const TileGrid& grid, TileCoord from, TileCoord to);

};  // namespace v3d::grid

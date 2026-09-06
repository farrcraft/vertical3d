/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "TileGrid.h"

#include <functional>
#include <vector>

namespace v3d::grid {

/**
 * Whether a tile stops sight passing through it, over and above the grid's own cover.
 *
 * Supplied by the caller so that nothing here learns what an occupant is, exactly as
 * TileFilter is for movement: smoke or a closed shutter is an extra blocker the map does not
 * carry. A default constructed blocker adds nothing, leaving sight to the grid alone.
 *
 * A blocker that is not symmetric in its own right - one that answers differently depending
 * on who is asking - breaks the guarantee below. It is asked about a tile and nothing else
 * for that reason.
 **/
typedef std::function<bool(TileCoord)> SightBlocker;

/**
 * Whether nothing blocks sight between two tiles.
 *
 * The relation is symmetric: hasLineOfSight(a, b) and hasLineOfSight(b, a) are the same
 * call. The two endpoints are put in a fixed order before anything is traced, so one line is
 * tested for both directions and there is no tie for the direction of travel to break
 * differently. Nothing is ever seen by something it cannot be seen by.
 *
 * Only Cover::Full on the grid blocks, and the two endpoints are never tested - an occupant
 * standing in cover can see out of it, the same way findPath() never tests the tile the
 * mover is standing on. Sight between a tile and itself, or between neighbours, is therefore
 * always clear.
 *
 * Sight squeezes through a corner exactly as movement does: where the line passes precisely
 * through the point four tiles share, it is stopped only if both tiles it passes between
 * block. Rounding the end of a wall sees past nothing.
 *
 * Off grid endpoints have no sight, so this needs no separate bounds test.
 **/
bool hasLineOfSight(const TileGrid& grid, TileCoord from, TileCoord to, const SightBlocker& blocks = {});

/**
 * The tiles the sight line passes through, from first and to last.
 *
 * Every tile the segment between the two centres touches, so consecutive entries are one 8
 * way step apart and the sequence is what a shot travels along: this is what an overlay
 * draws and what a pass through rule walks. Empty when either endpoint is off the grid; one
 * tile long when they are the same.
 *
 * Reversing the arguments reverses the result exactly, because the same trace serves both
 * directions.
 *
 * Nothing here says whether sight is clear. Where the line squeezes through a corner the two
 * tiles it passes between are absent from this sequence - that is the point of the corner
 * rule - so testing these tiles for blockers answers a different question than
 * hasLineOfSight() does. Ask that instead.
 **/
std::vector<TileCoord> sightLine(const TileGrid& grid, TileCoord from, TileCoord to);

};  // namespace v3d::grid

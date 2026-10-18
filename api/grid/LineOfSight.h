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
 * The caller supplies it, as it supplies a TileFilter for movement, so this library has no
 * concept of an occupant. A blocker can add smoke or a closed shutter, which the map does not
 * carry. A default-constructed blocker adds nothing, and the grid's cover alone decides.
 *
 * A blocker receives only the tile, so its result cannot depend on which endpoint is looking.
 * A result that did would break the symmetry of hasLineOfSight().
 **/
typedef std::function<bool(TileCoord)> SightBlocker;

/**
 * Whether nothing blocks sight between two tiles.
 *
 * The relation is symmetric: hasLineOfSight(a, b) equals hasLineOfSight(b, a). The two
 * endpoints are put in a fixed order before the line is traced, so both directions test the
 * same line, and no tie can be broken differently for each direction.
 *
 * Of the grid's cover, only Cover::Full blocks. The two endpoints are never tested, so an
 * occupant standing in cover can see out of it. findPath() likewise never tests the mover's
 * own tile. Sight between a tile and itself, or between orthogonal neighbours, is therefore
 * always clear.
 *
 * Sight passes a corner by the same rule as movement. Where the line passes exactly through
 * the point four tiles share, it is stopped only if both tiles it passes between block. A
 * line past the end of a wall is therefore not stopped by it. Diagonal neighbours have no
 * sight when both tiles between them block.
 *
 * An endpoint off the grid has no sight, so a caller needs no separate bounds test.
 **/
bool hasLineOfSight(const TileGrid& grid, TileCoord from, TileCoord to, const SightBlocker& blocks = {});

/**
 * The tiles the sight line passes through, from first and to last.
 *
 * It holds every tile the segment between the two centres passes through, so consecutive
 * entries are one 8-way step apart. It is the path a shot travels along, for an overlay to
 * draw or for a rule applied to each tile passed through. It is empty when either endpoint is
 * off the grid, and one tile long when the endpoints are the same.
 *
 * Reversing the arguments reverses the result exactly, because the same trace serves both
 * directions.
 *
 * sightLine() does not say whether sight is clear. Where the line passes exactly through a
 * corner, the two tiles it passes between are absent from the sequence. Testing these tiles
 * for blockers therefore gives a different answer from hasLineOfSight(). Call that instead.
 **/
std::vector<TileCoord> sightLine(const TileGrid& grid, TileCoord from, TileCoord to);

};  // namespace v3d::grid

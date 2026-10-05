/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "TileCoord.h"

#include <functional>

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

};  // namespace v3d::grid

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
 * Supplied by the caller so that this library has no notion of a mover. An app passes a
 * predicate over whatever it tracks, such as occupancy, ownership or a zone the mover must not
 * cross, and the grid stays the same type for all of them. A default constructed filter imposes
 * no restriction, leaving passability to the grid alone.
 **/
typedef std::function<bool(TileCoord)> TileFilter;

};  // namespace v3d::grid

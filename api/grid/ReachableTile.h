/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "TileCoord.h"

namespace v3d::grid {

/**
 * A tile a flood fill reached, and what reaching it cost.
 **/
struct ReachableTile {
    TileCoord tile;
    int cost{0};
};

};  // namespace v3d::grid

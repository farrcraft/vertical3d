/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "TileCoord.h"

#include <vector>

namespace v3d::grid {

/**
 * A glyph a legend did not name, and every tile it stands on in row order.
 **/
struct Unknown final {
    char glyph{0};
    std::vector<TileCoord> tiles;
};

};  // namespace v3d::grid

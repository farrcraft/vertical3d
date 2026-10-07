/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "TileGrid.h"

namespace v3d::grid {

/**
 * What a glyph in a map picture makes the tile it stands on.
 **/
struct Terrain final {
    bool passable{true};
    Cover cover{Cover::None};
};

};  // namespace v3d::grid

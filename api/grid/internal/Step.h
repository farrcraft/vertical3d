/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/grid/TileCoord.h>

#include <utility>

namespace v3d::grid::internal {

/**
 * One tile the traced segment moves onto.
 **/
struct Step {
    TileCoord tile;

    /**
     * Set when the segment passed exactly through the point four tiles share, so it moved
     * diagonally and touched neither tile it passed between. Those two tiles block sight
     * only when both of them block.
     **/
    bool diagonal{false};
    std::pair<TileCoord, TileCoord> corner;
};

};  // namespace v3d::grid::internal

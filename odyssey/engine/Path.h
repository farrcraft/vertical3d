/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <api/grid/TileGrid.h>

#include <cstddef>
#include <vector>

namespace odyssey::engine {

/**
 * The route an entity is walking, and how far into it it has got.
 *
 * The tiles are what v3d::grid::findPath returned, so the first of them is the tile the
 * entity was standing on when the route was asked for and `next` starts at 1. A path whose
 * tiles are empty is one that has been walked or was never found; the movement system
 * empties it rather than removing the component, so nothing is erased from under a view
 * that is iterating it.
 **/
struct Path final {
    std::vector<v3d::grid::TileCoord> tiles;

    /**
     * Index of the tile being walked towards.
     **/
    std::size_t next{0};

    /**
     * Seconds spent on the step in progress. Movement is a tile at a time rather than a
     * position between two, so this accumulates until a whole step is due.
     **/
    float elapsed{0.0f};
};

};  // namespace odyssey::engine

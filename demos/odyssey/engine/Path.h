/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <api/grid/TileCoord.h>

#include <cstddef>
#include <vector>

namespace odyssey::engine {

/**
 * The route an entity is walking, and how far into it it has got.
 *
 * The tiles are the route v3d::grid::findPath returned, in world tiles. The first is the tile
 * the entity stood on when the route was found, so `next` starts at 1. A path with no tiles
 * has been walked to its end, or no route was found. The movement system empties a finished
 * path instead of removing the component, because it is iterating a view that holds it.
 **/
struct Path final {
    std::vector<v3d::grid::TileCoord> tiles;

    /**
     * Index of the tile being walked towards.
     **/
    std::size_t next{0};

    /**
     * Seconds spent on the step in progress. An entity moves a whole tile at a time, so this
     * accumulates until the next tile is due.
     **/
    float elapsed{0.0F};
};

};  // namespace odyssey::engine

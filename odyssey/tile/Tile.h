/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

namespace odyssey::tile {

/**
 * What a tile is made of.
 *
 * This is the app's own vocabulary, not the grid's. `v3d::grid::TileGrid` holds only what
 * it needs to answer a route - whether a tile may be walked on and how much cover it
 * carries - and a kind is what decides those two and what the renderer draws. Two kinds
 * that block movement can still look nothing alike.
 **/
enum class Kind : std::uint8_t {
    Floor,  ///< Open ground. Walkable, no cover.
    Wall,   ///< Solid and head high. Blocks movement and sight.
    Crate   ///< Chest high. Blocks movement, and sight passes over it.
};

};  // namespace odyssey::tile

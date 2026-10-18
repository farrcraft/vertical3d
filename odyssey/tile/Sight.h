/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <api/grid/TileCoord.h>
#include <api/grid/TileGrid.h>

#include <cstddef>
#include <vector>

namespace odyssey::tile {

/**
 * What can be seen from where something stands, and what has been seen before.
 *
 * Each tile has two flags: whether it is in sight now, and whether it has ever been seen.
 * A wall the player has walked past is still known to be a wall when the player is
 * elsewhere, but only what is in sight now is current. A tile is therefore in sight,
 * remembered, or neither.
 *
 * Whether two tiles can see each other is decided by v3d::grid::hasLineOfSight, which is
 * symmetric. Only Cover::Full stops a line, so a crate is seen over. This class adds a range
 * and a memory, which the grid does not have.
 **/
class Sight final {
 public:
    /**
     * How far sight reaches, in tiles. The grid measures distance in Chebyshev
     * steps, where a diagonal step counts as one, so the tiles within RANGE form
     * a square of side 2 * RANGE + 1. This is the same unit as a movement budget.
     *
     * On a board twenty tiles wide, a range of seven leaves the far side unseen
     * until the player walks towards it.
     **/
    static constexpr int RANGE = 7;

    /**
     * Look from a tile, replacing what was in sight and adding to what is remembered.
     *
     * Every call traces sight again, even when the viewer has not moved, so a change to the
     * board is seen at once. Only the tiles within RANGE are traced, so a look costs a few
     * hundred short lines. A board of another size is treated as a different board, and
     * replaces what is remembered.
     **/
    void look(const v3d::grid::TileGrid& grid, v3d::grid::TileCoord from);

    /**
     * Put every tile out of sight, keeping what is remembered, for a board the viewer is not on.
     **/
    void blind();

    /**
     * @return whether a tile is in sight from where the last look() was taken. False for a
     *         tile off the board, and for every tile before the first look
     **/
    bool visible(v3d::grid::TileCoord tile) const;

    /**
     * @return whether a tile has ever been in sight
     **/
    bool remembered(v3d::grid::TileCoord tile) const;

 private:
    /**
     * @return the row major index of a tile, or the size of the board for one off it, so a
     *         caller tests the result rather than the bounds
     **/
    std::size_t index(v3d::grid::TileCoord tile) const;

    int width_{0};
    int height_{0};

    /**
     * Row major, indexed by index(), as remembered_ is. A byte per tile rather than
     * std::vector<bool>, whose proxy references are awkward at every call site.
     **/
    std::vector<char> visible_;
    std::vector<char> remembered_;
};

};  // namespace odyssey::tile

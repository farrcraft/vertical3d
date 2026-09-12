/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <api/grid/TileGrid.h>

#include <cstddef>
#include <vector>

namespace odyssey::tile {

/**
 * What can be seen from where something stands, and what has been seen before.
 *
 * Two answers per tile rather than one, because a board drawn from the first alone is a
 * board that forgets: a wall the player has walked past is still known to be a wall when
 * the player is elsewhere, and only what is in sight right now is current. A tile is
 * therefore in sight, remembered, or neither.
 *
 * Whether two tiles can see each other is v3d::grid::hasLineOfSight's answer, under
 * [ADR-0029](../../docs/adr/0029-tile-grids-are-an-api-library.md): only Cover::Full stops a
 * line, so a crate is seen over. What this adds is the range and the memory, neither of
 * which the grid has an opinion about.
 **/
class Sight final {
 public:
    /**
     * How far sight reaches. A box of side 2R + 1 is exactly the Chebyshev disc the grid
     * measures in, so this is a range in the same units a movement budget would be.
     *
     * Seven tiles on a board twenty wide leaves the far side of it to be walked to rather
     * than read from where the player started.
     **/
    static constexpr int RANGE = 7;

    /**
     * Look from a tile, replacing what was in sight and adding to what is remembered.
     *
     * Traced afresh every call rather than kept until the viewer moves: only the range box
     * is walked, so a look is a few hundred short lines, and a view kept would not notice
     * the board changing under it. A board of another size replaces what is remembered,
     * because it is another board.
     **/
    void look(const v3d::grid::TileGrid& grid, v3d::grid::TileCoord from);

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
     * Row major over the same index() as remembered_. A byte per tile, for the reason
     * TileGrid keeps its own flags that way.
     **/
    std::vector<char> visible_;
    std::vector<char> remembered_;
};

};  // namespace odyssey::tile

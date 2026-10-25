/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "LineOfSight.h"
#include "TileCoord.h"
#include "TileGrid.h"

#include <cstddef>
#include <vector>

namespace v3d::grid {

/**
 * What can be seen from where something stands, and what has been seen before.
 *
 * Each tile has two flags: whether it is in sight now, and whether it has ever been seen.
 * A wall the viewer has walked past is still known to be a wall when the viewer is
 * elsewhere, but only what is in sight now is current. A tile is therefore in sight,
 * remembered, or neither.
 *
 * Whether two tiles can see each other is decided by hasLineOfSight(), which is symmetric.
 * Only Cover::Full stops a line, plus whatever a SightBlocker adds. This class adds a range
 * and a memory, which the grid does not have.
 **/
class Sight final {
 public:
    /**
     * @param range how far sight reaches, in tiles. Distance is in Chebyshev steps, where a
     *        diagonal step counts as one, so the tiles within range form a square of side
     *        2 * range + 1. This is the same unit as a movement budget. A range below zero
     *        sees nothing, not even the viewer's own tile
     **/
    explicit Sight(int range);

    int range() const noexcept;

    /**
     * Look from a tile, replacing what was in sight and adding to what is remembered.
     *
     * Every call traces sight again, even when the viewer has not moved, so a change to the
     * board is seen at once. Only the tiles within range are traced. A board of another size
     * is treated as a different board, and replaces what is remembered.
     *
     * @param blocks tiles that stop sight on top of the grid's cover, as hasLineOfSight()
     *        takes them
     **/
    void look(const TileGrid& grid, TileCoord from, const SightBlocker& blocks = {});

    /**
     * Put every tile out of sight, keeping what is remembered, for a board the viewer is not on.
     **/
    void blind();

    /**
     * @return whether a tile is in sight from where the last look() was taken. False for a
     *         tile off the board, and for every tile before the first look
     **/
    bool visible(TileCoord tile) const;

    /**
     * @return whether a tile has ever been in sight
     **/
    bool remembered(TileCoord tile) const;

 private:
    /**
     * @return the row major index of a tile, or the size of the board for one off it, so a
     *         caller tests the result rather than the bounds
     **/
    std::size_t index(TileCoord tile) const;

    int range_;
    int width_{0};
    int height_{0};

    /**
     * Row major, indexed by index(), as remembered_ is. A byte per tile rather than
     * std::vector<bool>, whose proxy references are awkward at every call site.
     **/
    std::vector<char> visible_;
    std::vector<char> remembered_;
};

};  // namespace v3d::grid

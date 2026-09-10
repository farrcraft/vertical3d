/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/kind/Json.h>
#include <api/grid/TileGrid.h>
#include <api/log/Logger.h>

#include <vector>

#include "Tile.h"

#include <boost/shared_ptr.hpp>

namespace odyssey::tile {

/**
 * The board odyssey plays on: a v3d::grid::TileGrid, what each of its tiles is made of,
 * and where the player starts.
 *
 * The document is data/map.json, a "tiles" array of equal length strings with one
 * character per tile and the first string the northmost row. The characters are:
 *
 *     '.'  floor
 *     '#'  wall
 *     'o'  crate
 *     '@'  floor, and where the player starts
 *
 * Rows of characters rather than an array of objects because a map is read far more often
 * by a person than by the program, and this way the file looks like the board. The format
 * is odyssey's own and lives here rather than in api/grid: the grid is a data structure
 * with no opinion about where a map came from, and one app wanting a file is not a library
 * ([ADR-0016](../../docs/adr/0016-undo-records-what-has-already-happened.md)). It earns a
 * record of its own when something other than this app reads or writes one.
 **/
class Map final {
 public:
    /**
     * @param logger where a malformed document is reported
     **/
    explicit Map(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Read a map document, replacing whatever was loaded before.
     *
     * A map with no rows, with rows of differing lengths, or with a character that names
     * no kind is rejected rather than patched: a map that loaded as something other than
     * what was written is worse than one that did not load.
     *
     * @return whether the document was understood
     **/
    bool load(const boost::shared_ptr<v3d::asset::kind::Json>& document);

    /**
     * @return whether a map has been loaded, and so whether grid() may be dereferenced
     **/
    bool loaded() const noexcept;

    /**
     * The grid to route over. Its passability and cover come from the tile kinds: a wall
     * is impassable and Cover::Full, a crate is impassable and Cover::Half, floor is
     * passable and Cover::None.
     **/
    const boost::shared_ptr<v3d::grid::TileGrid>& grid() const noexcept;

    /**
     * @return what a tile is made of, or Kind::Wall for a tile off the map - nothing can
     *         stand outside the board, and answering rather than throwing keeps this the
     *         same shape as TileGrid::passable()
     **/
    Kind kind(v3d::grid::TileCoord tile) const;

    /**
     * Where the player starts: the '@' in the document, or the first floor tile when the
     * document names none.
     **/
    v3d::grid::TileCoord start() const noexcept;

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<v3d::grid::TileGrid> grid_;
    std::vector<Kind> kinds_;  ///< row major over the same order as the document's rows
    v3d::grid::TileCoord start_;
};

};  // namespace odyssey::tile

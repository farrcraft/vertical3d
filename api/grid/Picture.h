/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "TileGrid.h"

namespace v3d::grid {

/**
 * What a glyph in a map picture makes the tile it stands on.
 **/
struct Terrain final {
    bool passable{true};
    Cover cover{Cover::None};
};

/**
 * A glyph a legend did not name, and every tile it stands on in row order.
 **/
struct Unknown final {
    char glyph{0};
    std::vector<TileCoord> tiles;
};

/**
 * A grid read from a picture, or why it could not be.
 **/
struct Picture final {
    std::optional<TileGrid> grid;  ///< empty when the picture was refused, with the reason in error

    /**
     * Every glyph the legend does not name, in the order each first appears. Its tiles are
     * left impassable with no cover, for the caller to set if the glyph means something to it.
     **/
    std::vector<Unknown> unknown;

    std::string error;
};

/**
 * Build a grid from rows of glyphs, one per tile, and a legend saying what each glyph is.
 *
 * Only the terrain is the grid's - ADR-0062. The file the rows came from, and whatever else a
 * glyph means to a game, such as a prop, a spawn or where the player starts, are the game's. So
 * a glyph the legend does not name is handed back rather than refused. A game that treats one
 * as an error does so on top.
 *
 * @param rows the picture, row y of it being tile row y, all the same length
 * @param legend what each glyph is made of
 * @param tileSize the grid's tile edge length in world units
 * @return the grid, or no grid and the reason when there are no rows, a row is empty, or the
 *         rows differ in length - none of which has a tile to hand back
 **/
Picture fromPicture(const std::vector<std::string>& rows, const std::map<char, Terrain>& legend,
    float tileSize = TileGrid::DEFAULT_TILE_SIZE);

};  // namespace v3d::grid

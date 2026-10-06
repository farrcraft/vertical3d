/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Terrain.h"
#include "TileGrid.h"
#include "Unknown.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace v3d::grid {

/**
 * A grid read from a picture, or why it could not be.
 **/
struct Picture final {
    std::optional<TileGrid> grid;  /**< empty when the picture was refused, with the reason in error **/

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
 * The grid parses terrain only. Reading the file the rows came from, and any other meaning a
 * glyph has to a game, such as a prop, a spawn or the player's start, belong to the game. A
 * glyph the legend does not name is therefore returned in Picture::unknown rather than
 * refused. A game that treats one as an error checks that list itself.
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

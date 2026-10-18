/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/kind/Json.h>
#include <api/grid/TileCoord.h>
#include <api/grid/TileGrid.h>
#include <api/log/Logger.h>

#include <optional>
#include <vector>

#include "Tile.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

namespace odyssey::tile {

/**
 * The board odyssey plays on: a v3d::grid::TileGrid, what each of its tiles is made of,
 * and where the player starts.
 *
 * A map document, such as data/map.json, holds a "tiles" array of equal length strings. Each
 * character is a tile, and the first string is the northmost row. The characters are:
 *
 *     '.'  floor
 *     '#'  wall
 *     'o'  crate
 *     '@'  floor, and where the player starts
 *
 * The rows are text so that the file looks like the board. v3d::grid::fromPicture builds the
 * grid from the rows and the terrain of '.', '#' and 'o'. The JSON around the rows, the kinds
 * and the '@' are odyssey's own. api/grid parses only a picture against a terrain legend.
 **/
class Map final {
 public:
    /**
     * A tile's edge in world units. One, so that a world position's whole part names its tile.
     **/
    static constexpr float TILE_SIZE = 1.0F;

    /**
     * @param logger where a malformed document is reported
     **/
    explicit Map(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Read a map document, replacing whatever was loaded before.
     *
     * A map with no rows, with rows of differing lengths, with a character that names no
     * kind, or with no floor to stand on is rejected, not patched. A rejected document leaves
     * the map that was loaded before it in place.
     *
     * @param corner where the grid's minimum corner is placed in the world, for a region; the
     *        grid is centred on the origin without one
     * @return whether the document was understood
     **/
    bool load(const boost::shared_ptr<v3d::asset::kind::Json>& document, const std::optional<glm::vec3>& corner = std::nullopt);

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
     * @return what a tile is made of, or Kind::Wall for a tile off the map. Like
     *         TileGrid::passable(), it does not throw for a tile off the map
     **/
    Kind kind(v3d::grid::TileCoord tile) const;

    /**
     * Where the player starts: the last '@' in the document, or the first floor tile when the
     * document has none.
     **/
    v3d::grid::TileCoord start() const noexcept;

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<v3d::grid::TileGrid> grid_;
    std::vector<Kind> kinds_;  ///< row major over the same order as the document's rows
    v3d::grid::TileCoord start_;
};

};  // namespace odyssey::tile

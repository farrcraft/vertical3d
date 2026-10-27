/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/asset/Worker.h>
#include <api/asset/kind/Json.h>
#include <api/grid/GridSet.h>
#include <api/grid/Sight.h>
#include <api/grid/TileCoord.h>
#include <api/log/Logger.h>
#include <demos/odyssey/tile/Map.h>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace odyssey::tile {

/**
 * The world odyssey plays in: regions laid side by side, each a map of its own, loaded while the
 * player approaches it and released once the player is far from it.
 *
 * Positions are world tiles. A tile is one world unit, so world tile (x, y) covers world X from x
 * to x + 1 and world Z from y to y + 1. Each region's grid is placed by its corner, so a region's
 * own tile is the world tile less the region's corner.
 *
 * The document is data/world.json:
 *
 *     {
 *       "regions": [
 *         { "file": "map.json",  "corner": { "x": 0,  "y": 0 }, "size": { "x": 20, "y": 12 } },
 *         { "file": "east.json", "corner": { "x": 20, "y": 0 }, "size": { "x": 20, "y": 12 } }
 *       ],
 *       "start": 0
 *     }
 *
 * Each region names its map file, its corner in world tiles, and its size in tiles, which its
 * map has to match. Regions may not overlap. "start" is the region the player starts in, at the
 * start its map names.
 *
 * A route and what the player sees stay within the region the player stands in. A step from one
 * region's edge onto the next region's first tile crosses between them, once that region is
 * loaded.
 **/
class World final {
 public:
    /**
     * The name config.json gives the kind of a world document.
     **/
    static constexpr std::string_view DOCUMENT = "world";

    /**
     * How near, in tiles, the player comes to a region before it is requested.
     **/
    static constexpr int APPROACH = 4;

    /**
     * How far, in tiles, the player goes from a region before it is released. It is more than
     * APPROACH, so a player standing at the boundary does not load and release a region on
     * alternate steps.
     **/
    static constexpr int DEPART = 10;

    /**
     * How far the player sees, in tiles. On a board twenty tiles wide, a range of seven leaves
     * the far side unseen until the player walks towards it.
     **/
    static constexpr int SIGHT_RANGE = 7;

    /**
     * @param logger where a malformed document or a region that does not load is reported
     * @param assets where the region maps are read from, on the worker thread as well as this one
     * @param worker loads every region but the start region, on its own thread
     **/
    World(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::asset::Manager>& assets,
        const boost::shared_ptr<v3d::asset::Worker>& worker);

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    /**
     * Read a world document and load its start region at once, replacing any world loaded
     * before.
     *
     * @return whether the document was understood and the start region loaded
     **/
    bool load(const boost::shared_ptr<v3d::asset::kind::Json>& document);

    /**
     * @return where the player starts, in world tiles
     **/
    v3d::grid::TileCoord start() const noexcept;

    /**
     * @return how many regions the world has, loaded or not
     **/
    std::size_t regions() const noexcept;

    /**
     * @return the region a world tile lies in, whether or not it is loaded, or nothing for a tile
     *         in no region
     **/
    std::optional<std::size_t> regionAt(v3d::grid::TileCoord tile) const;

    /**
     * @return a region's map, or null while it is not loaded
     **/
    boost::shared_ptr<const Map> map(std::size_t region) const;

    /**
     * @return what the player can see of a region, and remembers of it. It outlives the region's
     *         map, so a region released and loaded again is remembered as it was
     **/
    const v3d::grid::Sight& sight(std::size_t region) const;

    /**
     * @return a region's corner in world tiles
     **/
    v3d::grid::TileCoord corner(std::size_t region) const;

    /**
     * @return the region's own tile for a world tile
     **/
    v3d::grid::TileCoord local(std::size_t region, v3d::grid::TileCoord tile) const;

    /**
     * @return the world tile for one of the region's own tiles
     **/
    v3d::grid::TileCoord world(std::size_t region, v3d::grid::TileCoord tile) const;

    /**
     * @return whether a world tile can be entered: it is in a loaded region, and that region's
     *         grid says it is passable
     **/
    bool passable(v3d::grid::TileCoord tile) const;

    /**
     * Every loaded region's grid, as one board in world tiles, for a route or a line of sight
     * that crosses a seam. A region that is not loaded is a wall to it. It names the maps loaded
     * now, so it is built again for each search rather than kept.
     **/
    v3d::grid::GridSet grids() const;

    /**
     * @return the least x and y, and one past the greatest, of every region's world tiles
     **/
    v3d::grid::TileCoord min() const noexcept;
    v3d::grid::TileCoord max() const noexcept;

    /**
     * Request each region the player has come within APPROACH tiles of, and release each loaded
     * region the player is more than DEPART tiles from. The region the player stands in is never
     * released. A region whose map could not be read is asked for again only after the player
     * has gone DEPART tiles from it.
     **/
    void stream(v3d::grid::TileCoord player);

    /**
     * Look from where the player stands, across every loaded region, so the player sees past a
     * seam into the region beside it. A region that is not loaded is out of sight, and keeps
     * only what it remembers.
     **/
    void look(v3d::grid::TileCoord player);

    /**
     * The world tile at the top left of a screen that shows a span of tiles with the player as
     * near its middle as the world's edges allow. A world narrower than the screen is drawn from
     * its least tile.
     *
     * @param screen how many tiles the screen shows across and down
     **/
    v3d::grid::TileCoord view(v3d::grid::TileCoord player, v3d::grid::TileCoord screen) const;

    /**
     * @return whether a region's map is loaded
     **/
    bool loaded(std::size_t region) const;

    /**
     * @return whether a region is being loaded on the worker
     **/
    bool loading(std::size_t region) const;

 private:
    /**
     * One region: where it is, its map once loaded, and what has been seen of it.
     **/
    struct Region final {
        std::string file;
        v3d::grid::TileCoord corner;
        v3d::grid::TileCoord size;
        boost::shared_ptr<const Map> map;
        v3d::grid::Sight sight{SIGHT_RANGE};
        bool loading = false;
        /**
         * Its map could not be read, so it is not asked for again until the player has gone
         * DEPART tiles from it.
         **/
        bool failed = false;
        /**
         * Incremented by each request and each release. A load that finishes after the region
         * was released, or requested again, carries an older value and is dropped.
         **/
        unsigned int generation = 0;
    };

    /**
     * Read a region's map, on whatever thread calls it.
     *
     * @return the map, or null when it could not be read or does not match the region's size
     **/
    static boost::shared_ptr<const Map> read(const boost::shared_ptr<v3d::log::Logger>& logger,
        const boost::shared_ptr<v3d::asset::Manager>& assets, const std::string& file, v3d::grid::TileCoord corner,
        v3d::grid::TileCoord size);

    /**
     * @return how many tiles a world tile is from a region, along the farther of the two axes,
     *         and zero for a tile inside it
     **/
    static int distance(const Region& region, v3d::grid::TileCoord tile);

    void request(std::size_t index);
    void release(std::size_t index);

    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<v3d::asset::Manager> assets_;
    boost::shared_ptr<v3d::asset::Worker> worker_;
    /**
     * A shared_ptr, because each completion holds a weak_ptr to it. A completion that runs after
     * the world is destroyed, or after load() replaces the regions, finds nothing to fill.
     **/
    boost::shared_ptr<std::vector<Region>> regions_;
    v3d::grid::TileCoord start_{0, 0};
    v3d::grid::TileCoord min_{0, 0};
    v3d::grid::TileCoord max_{0, 0};
};

};  // namespace odyssey::tile

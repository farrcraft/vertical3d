/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "TileCoord.h"
#include "TileGrid.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::grid {

/**
 * Grids placed side by side, which a search treats as one board in world tiles.
 *
 * A world's regions are each a grid placed by its corner. A set holds references to the ones
 * loaded, and findPath(), reachableTiles(), DistanceField and hasLineOfSight() take it in place
 * of one grid. They then work in world tiles: world tile (x, y) is the tile whose minimum corner
 * is at world X = x * tileSize, Z = y * tileSize. A grid's tile (0, 0) is the world tile at its
 * origin(), and its other tiles follow on from there.
 *
 * **A tile in no grid of the set is impassable and blocks sight**, as a tile off one grid is.
 * A region that is not loaded is therefore a wall, and a route never plans through it.
 *
 * The set holds references, not copies. Build it from the grids that are loaded when a search
 * is made, and do not keep it past a grid's release. Every grid shares the movement rules of
 * one grid, so a diagonal step between two blocked tiles is refused even when they lie in
 * different grids.
 *
 * A grid joins a set only when its tile size is the set's, its corner lies on the world's tile
 * lattice, and it overlaps no grid already in the set. refusal() says why one cannot.
 **/
class GridSet final {
 public:
    /**
     * An empty set. A grid added first decides the tile size.
     **/
    GridSet() = default;

    /**
     * One grid, in its own coordinates rather than world tiles: its tile (0, 0) is the set's.
     * A search over this set is the search over the grid alone.
     **/
    static GridSet local(const TileGrid& grid);

    /**
     * Add a grid, placed by its corner.
     *
     * @throws std::invalid_argument with the reason refusal() gives
     **/
    void add(const TileGrid& grid);

    /**
     * Why a grid cannot join this set, or nothing when it can. A grid is refused for a tile
     * size that is not the set's, or a corner that is not a whole number of tiles from the
     * world's origin. It is also refused for a tile that a grid already in the set holds.
     **/
    std::optional<std::string> refusal(const TileGrid& grid) const;

    bool empty() const noexcept;

    /**
     * @return the tile size every grid in the set shares, or zero for an empty set
     **/
    float tileSize() const noexcept;

    /**
     * @return the world tile a grid's tile (0, 0) is, or nothing for a grid not in the set
     **/
    std::optional<TileCoord> origin(const TileGrid& grid) const;

    /**
     * @return the grid holding a world tile, or null for a tile in none of them
     **/
    const TileGrid* gridAt(TileCoord tile) const;

    /**
     * @return whether a grid in the set holds the world tile
     **/
    bool contains(TileCoord tile) const;

    /**
     * Whether a world tile may be walked on. A tile in no grid is impassable.
     **/
    bool passable(TileCoord tile) const;

    /**
     * Whether a world tile stops sight: one with Cover::Full, or one in no grid.
     **/
    bool opaque(TileCoord tile) const;

    /**
     * @return how many tiles the grids hold between them
     **/
    std::size_t count() const noexcept;

    /**
     * The position of a world tile's entry in an array holding one value per tile of the set.
     * A tile in no grid has no index, so a caller checks contains() first.
     **/
    std::size_t index(TileCoord tile) const;

    /**
     * @return the world tile at an index, the inverse of index()
     **/
    TileCoord tile(std::size_t index) const;

 private:
    /**
     * One grid of the set, where it sits in world tiles, and where its tiles start in the
     * set's index. The size is kept here, so index() and tile() read no grid.
     **/
    struct Placed final {
        const TileGrid* grid = nullptr;
        TileCoord origin{0, 0};
        int width = 0;
        int height = 0;
        std::size_t offset = 0;
    };

    /** The grid holding a world tile, or null. **/
    const Placed* find(TileCoord tile) const;

    /** Add a grid at an origin, in world tiles. **/
    void place(const TileGrid& grid, TileCoord origin);

    /** The world tile at a grid's tile (0, 0), or nothing when its corner is off the lattice. **/
    static std::optional<TileCoord> lattice(const TileGrid& grid);

    std::vector<Placed> grids_;
    std::size_t count_ = 0;
    float tileSize_ = 0.0F;
};

};  // namespace v3d::grid

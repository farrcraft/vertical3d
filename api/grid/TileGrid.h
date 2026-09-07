/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::grid {

/**
 * Integer address of a tile on the grid.
 *
 * x runs along world +X and y along world +Z: the grid lies flat, so the second axis is
 * depth, not height. Out of range values are allowed, because worldToTile() answers where a
 * point would be and the caller asks TileGrid::contains() whether that is on the map.
 **/
struct TileCoord {
    int x{0};
    int y{0};

    friend bool operator==(TileCoord, TileCoord) = default;
};

/**
 * How much of whatever stands on a tile is shielded by what is on it.
 *
 * Terrain rather than a rule: a wall and a crate carry cover for the same reason, and
 * neither knows what is standing behind it. Which cover applies to a particular line
 * depends on where the other end of that line is, and that belongs to whoever is using the
 * grid.
 *
 * Cover height and passability are independent flags and neither implies the other. The
 * combinations that make sense are worth stating rather than enforcing: a crate or a low
 * wall is Half and impassable, a doorway is None and passable, a solid wall is Full and
 * impassable, and open ground is None and passable.
 *
 * Declared in increasing height, so the best cover of a set is its maximum.
 **/
enum class Cover : std::uint8_t {
    None,  ///< Open ground. Shields nothing and stops nothing.
    Half,  ///< Chest high. Harder to hit past, but sight passes over it.
    Full  ///< Head high. The only value that blocks line of sight.
};

/**
 * The cover height's name, for readouts and error messages.
 **/
std::string_view toString(Cover cover);

/**
 * A rectangular grid of tiles on the Y = 0 plane: dimensions, tile and world conversion,
 * and per tile passability and cover.
 *
 * Pure data and arithmetic. It holds no entities and does not track what occupies a tile;
 * occupancy belongs to whatever is moving over the grid and is supplied to pathfinding
 * separately as a predicate, which is what keeps this type usable for map generation and
 * for a running simulation alike.
 *
 * The grid is centred on the world origin.
 **/
class TileGrid {
 public:
    /**
     * Tile edge length in world units, for a grid whose consumer does not choose one.
     *
     * 1.5 suits a figure around 1.8 tall: the figure is slightly taller than its tile is
     * wide, and a footprint of about 0.6 leaves clear ground on every side, so two
     * occupants of adjacent tiles read as two. At 1.0 a figure of that height overhangs
     * its own tile.
     **/
    static constexpr float DEFAULT_TILE_SIZE = 1.5f;

    /**
     * @param width tiles along world +X
     * @param height tiles along world +Z
     * @param tileSize tile edge length in world units
     * @throws std::invalid_argument if any dimension or the tile size is not positive
     **/
    TileGrid(int width, int height, float tileSize = DEFAULT_TILE_SIZE);

    int width() const;
    int height() const;
    float tileSize() const;

    /**
     * @return whether a coordinate addresses a tile that exists
     **/
    bool contains(TileCoord tile) const;

    /**
     * @return the world position of a tile's centre, on the ground plane, which is where
     *         anything standing on the tile sits
     **/
    glm::vec3 tileToWorld(TileCoord tile) const;

    /**
     * The tile a world position falls in, ignoring Y.
     *
     * Floors rather than rounds, so the mapping is uniform either side of the origin and a
     * point on the boundary between two tiles belongs to the higher one. The result is not
     * clamped: a point off the map returns an out of bounds coordinate, so picking can tell
     * "off the grid" from "the edge tile".
     **/
    TileCoord worldToTile(const glm::vec3& position) const;

    /**
     * @return the world position of the grid's minimum corner, half a tile out from tile
     *         (0, 0)'s centre on both axes
     **/
    glm::vec3 worldMin() const;

    /**
     * Whether a tile may be walked on. Off grid tiles are impassable, so a path search
     * needs no separate bounds test.
     **/
    bool passable(TileCoord tile) const;

    /**
     * Mark a tile passable or blocked.
     *
     * @throws std::out_of_range if the tile is not on the grid. Reading off the map is
     *         allowed; writing off it is always a mistake.
     **/
    void setPassable(TileCoord tile, bool passable);

    /**
     * How much cover a tile carries.
     *
     * Off grid tiles have no cover. Nothing off the map can shield what is on it, and
     * answering rather than throwing keeps this the same shape as passable().
     **/
    Cover cover(TileCoord tile) const;

    /**
     * Set a tile's cover height.
     *
     * Passability is left alone - the two are independent, and a caller placing a wall sets
     * both.
     *
     * @throws std::out_of_range if the tile is not on the grid, for the same reason
     *         setPassable() does
     **/
    void setCover(TileCoord tile, Cover cover);

 private:
    std::size_t index(TileCoord tile) const;

    int width_{0};
    int height_{0};
    float tileSize_{DEFAULT_TILE_SIZE};

    /**
     * Row major, y * width + x. A byte per tile rather than std::vector<bool>, whose proxy
     * references are awkward at every call site.
     **/
    std::vector<char> passable_;

    /**
     * Row major over the same index() as passable_.
     **/
    std::vector<Cover> cover_;
};

};  // namespace v3d::grid

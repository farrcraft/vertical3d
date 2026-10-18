/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "TileCoord.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::grid {

/**
 * How tall the cover on a tile is.
 *
 * Cover is terrain, not a rule. A wall and a crate carry cover for the same reason, and
 * neither depends on what stands behind it. Whether cover applies to a particular line
 * depends on where the other end of the line is, and the code using the grid decides that.
 *
 * Cover height and passability are independent, and neither implies the other. The sensible
 * combinations are listed here, not enforced. A crate or a low wall is Half
 * and impassable. A doorway is None and passable. A solid wall is Full and impassable. Open
 * ground is None and passable.
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
 * Pure data and arithmetic. It holds no entities and does not track what occupies a tile. The
 * caller tracks occupancy and passes it to pathfinding as a TileFilter. The same type
 * therefore serves map generation and a running simulation.
 *
 * The grid is centred on the world origin unless it is given the world position of its minimum
 * corner. A region of a larger world is a grid placed by its corner. A grid holds no reference
 * to other regions, and two grids placed to overlap are not detected.
 **/
class TileGrid {
 public:
    /**
     * Tile edge length in world units, for a grid whose consumer does not choose one.
     *
     * 1.5 suits a figure about 1.8 tall. The figure is slightly taller than its tile is
     * wide, and a footprint of about 0.6 leaves clear ground on every side, so occupants of
     * adjacent tiles read as two figures. At 1.0, a figure of that height overhangs its own
     * tile.
     **/
    static constexpr float DEFAULT_TILE_SIZE = 1.5F;

    /**
     * @param width tiles along world +X
     * @param height tiles along world +Z
     * @param tileSize tile edge length in world units
     * @throws std::invalid_argument with the reason refusal() gives
     **/
    TileGrid(int width, int height, float tileSize = DEFAULT_TILE_SIZE);

    /**
     * A grid placed in the world by its minimum corner: the corner of tile (0, 0) with the
     * lowest world X and Z.
     *
     * @param corner the corner's world position. Its y is ignored, because the grid lies on the
     *        Y = 0 plane
     * @throws std::invalid_argument with the reason refusal() gives
     **/
    TileGrid(int width, int height, float tileSize, const glm::vec3& corner);

    /**
     * Why a grid cannot be made of these, or nothing when it can.
     *
     * A grid is refused for a dimension that is not positive, or a tile size that is not a finite
     * positive number. It is also refused for a tile size that makes a side too long to hold in a
     * float, or a corner whose x or z is not finite.
     *
     * @param corner the corner a placed grid is given; a centred grid passes the origin
     **/
    static std::optional<std::string> refusal(int width, int height, float tileSize, const glm::vec3& corner);

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
     * It floors, not rounds, so the mapping is uniform on both sides of the minimum corner,
     * and a point on the boundary between two tiles belongs to the higher one. The result is
     * not clamped. A point off the map returns a coordinate off the grid, so picking can tell
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
     *         allowed. Writing off it is always a mistake.
     **/
    void setPassable(TileCoord tile, bool passable);

    /**
     * How much cover a tile carries.
     *
     * A tile off the grid has no cover. Like passable(), it returns a value for such a tile
     * instead of throwing.
     **/
    Cover cover(TileCoord tile) const;

    /**
     * Set a tile's cover height.
     *
     * Passability is unchanged. The two are independent, so a caller placing a wall sets
     * both.
     *
     * @throws std::out_of_range if the tile is not on the grid, for the same reason
     *         setPassable() does
     **/
    void setCover(TileCoord tile, Cover cover);

    /**
     * The position of a tile's entry in any array holding one value per tile: row major,
     * y * width + x. A tile off the grid has no index, so a caller checks contains() first.
     **/
    std::size_t index(TileCoord tile) const;

 private:
    int width_{0};
    int height_{0};
    float tileSize_{DEFAULT_TILE_SIZE};
    glm::vec3 corner_{0.0F};  /**< the world position of the minimum corner, on Y = 0 **/

    /**
     * Row major, y * width + x. A byte per tile, not std::vector<bool>, whose proxy
     * references are awkward to use.
     **/
    std::vector<char> passable_;

    /**
     * Row major over the same index() as passable_.
     **/
    std::vector<Cover> cover_;
};

};  // namespace v3d::grid

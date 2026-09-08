/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "TileGrid.h"

#include <array>
#include <functional>
#include <vector>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::grid {

/**
 * How far overlay geometry sits above the ground plane, in world units.
 *
 * A grid and the ground it is drawn on are both at y = 0, and coplanar geometry z-fights
 * wherever depth testing is on. A centimetre clears it. Everything drawn over one grid
 * shares the lift, or one layer of the overlay sinks into another.
 **/
constexpr float OVERLAY_LIFT = 0.01f;

/**
 * Where a segment of the overlay goes, and in what colour.
 *
 * The overlay is handed out as segments rather than written into a canvas, so that nothing
 * here names a renderer and every case can be asserted without a device - the seam
 * ComponentRenderer takes its text measuring across, for the same reason. A caller drawing
 * through the line primitive of [ADR-0011](../../docs/adr/0011-lines-are-the-second-primitive.md)
 * passes a sink of two lines.
 **/
typedef std::function<void(const glm::vec3& from, const glm::vec3& to, const glm::vec4& colour)> LineSink;

/**
 * Where a filled quad of the overlay goes, and in what colour.
 *
 * The counterpart of LineSink for the world space quad of
 * [ADR-0042](../../docs/adr/0042-a-textured-quad-in-world-space.md), and here for the same
 * reason: nothing in this library names a renderer. The corners arrive in the order
 * tileCorners() gives them, which is the order realtime::WorldCanvas takes them in.
 **/
typedef std::function<void(const std::array<glm::vec3, 4>& corners, const glm::vec4& colour)> QuadSink;

/**
 * The four corners of a tile, in order around its perimeter and lifted clear of the ground
 * plane by OVERLAY_LIFT.
 *
 * Ordered from the minimum corner and turning toward +z, so consecutive corners share an
 * edge and the fourth closes back onto the first.
 **/
std::array<glm::vec3, 4> tileCorners(const TileGrid& grid, TileCoord tile);

/**
 * The four edges of one tile, which is what a highlight under a cursor or a selection is
 * drawn as. A tile off the grid emits nothing.
 **/
void outlineTile(const TileGrid& grid, TileCoord tile, const glm::vec4& colour, const LineSink& sink);

/**
 * One tile filled, which is what a highlight under a cursor, a movement range or a
 * threatened square is drawn as. A tile off the grid emits nothing.
 *
 * The colour is what reaches the sink whole, alpha included: a highlight over ground that
 * has to stay visible is a translucent fill, and whether that blends is the pass's.
 **/
void fillTile(const TileGrid& grid, TileCoord tile, const glm::vec4& colour, const QuadSink& sink);

/**
 * Every tile of a run filled in one colour - a movement range, an area of effect, a
 * selection. Tiles off the grid are skipped rather than refusing the whole run.
 **/
void fillTiles(const TileGrid& grid, const std::vector<TileCoord>& tiles, const glm::vec4& colour,
    const QuadSink& sink);

/**
 * Every tile boundary of a grid, with the four outer edges in a colour of their own so the
 * extent of the board is legible against its interior.
 *
 * The lines are the boundaries rather than the tiles, so a w by h grid emits w + h + 2
 * segments rather than one per tile.
 **/
void outlineGrid(const TileGrid& grid, const glm::vec4& interior, const glm::vec4& border, const LineSink& sink);

};  // namespace v3d::grid

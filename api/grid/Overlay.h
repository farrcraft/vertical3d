/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "TileCoord.h"
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
 * here names a renderer and every case can be tested without a device. A caller drawing
 * through the renderer's world-space line primitive needs only a short sink that forwards
 * each segment to it.
 **/
typedef std::function<void(const glm::vec3& from, const glm::vec3& to, const glm::vec4& colour)> LineSink;

/**
 * Where a filled quad of the overlay goes, and in what colour.
 *
 * The counterpart of LineSink for the renderer's world-space quad, for the same reason:
 * nothing in this library names a renderer. The corners arrive in the order tileCorners()
 * returns them, which is also the order realtime::WorldCanvas expects.
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
 * The four edges of one tile, for drawing a highlight under a cursor or a selection. A tile
 * off the grid emits nothing.
 **/
void outlineTile(const TileGrid& grid, TileCoord tile, const glm::vec4& colour, const LineSink& sink);

/**
 * One tile filled, for drawing a highlight under a cursor, a movement range or a threatened
 * square. A tile off the grid emits nothing.
 *
 * The colour reaches the sink unchanged, alpha included, so a highlight that must leave the
 * ground visible is a translucent fill. Whether it blends is decided by the render pass.
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

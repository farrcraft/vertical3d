/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "TileGrid.h"

#include <array>
#include <functional>

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
 * Every tile boundary of a grid, with the four outer edges in a colour of their own so the
 * extent of the board is legible against its interior.
 *
 * The lines are the boundaries rather than the tiles, so a w by h grid emits w + h + 2
 * segments rather than one per tile.
 **/
void outlineGrid(const TileGrid& grid, const glm::vec4& interior, const glm::vec4& border, const LineSink& sink);

};  // namespace v3d::grid

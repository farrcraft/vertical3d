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
 * A grid and the ground it is drawn on are both at y = 0, and coplanar geometry z-fights when
 * depth testing is on. A lift of 0.01 clears it. Everything drawn over one grid uses the same
 * lift, so that no layer of the overlay sinks into another.
 **/
constexpr float OVERLAY_LIFT = 0.01F;

/**
 * Where a segment of the overlay goes, and in what colour.
 *
 * The overlay is given out as segments, not drawn into a canvas, so this library does not
 * depend on a renderer and its tests need no device. A caller that draws with the renderer's
 * world-space lines passes a sink that forwards each segment to them.
 **/
typedef std::function<void(const glm::vec3& from, const glm::vec3& to, const glm::vec4& colour)> LineSink;

/**
 * Where a filled quad of the overlay goes, and in what colour.
 *
 * The counterpart of LineSink for filled quads. The corners arrive in the order tileCorners()
 * returns them, which is also the order realtime::canvas::World expects.
 **/
typedef std::function<void(const std::array<glm::vec3, 4>& corners, const glm::vec4& colour)> QuadSink;

/**
 * The four corners of a tile, in order around its perimeter and lifted clear of the ground
 * plane by OVERLAY_LIFT.
 *
 * The corners start at the minimum corner and go toward +X first, then +Z, so consecutive
 * corners share an edge and the fourth closes back onto the first.
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
 * ground visible is a translucent fill. The render pass decides whether it blends.
 **/
void fillTile(const TileGrid& grid, TileCoord tile, const glm::vec4& colour, const QuadSink& sink);

/**
 * Every tile in a list filled in one colour, such as a movement range, an area of effect or a
 * selection. A tile off the grid is skipped, and the rest are still drawn.
 **/
void fillTiles(const TileGrid& grid, const std::vector<TileCoord>& tiles, const glm::vec4& colour,
    const QuadSink& sink);

/**
 * Every tile boundary of a grid. The four outer edges are drawn in border, so the edge of the
 * grid stands out from the interior lines.
 *
 * Each boundary is one segment across the whole grid, so a w by h grid emits w + h + 2
 * segments, not four per tile.
 **/
void outlineGrid(const TileGrid& grid, const glm::vec4& interior, const glm::vec4& border, const LineSink& sink);

};  // namespace v3d::grid

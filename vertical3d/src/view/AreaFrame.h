/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/canvas/Canvas.h>

#include <cstddef>
#include <optional>
#include <span>

#include "AreaLayout.h"

#include <glm/vec4.hpp>

namespace v3d::editor::view {

/**
 * The colours and sizes the frame around the areas is drawn in.
 **/
struct FrameLook final {
    static constexpr float RADIUS = 4.0F;

    glm::vec4 gap{ 0.0F };        /**< the gaps, and each area's corners outside its rounding **/
    glm::vec4 hairline{ 0.0F };   /**< the line just inside each area's edge **/
    glm::vec4 accent{ 0.0F };     /**< the active area's line, and a lit divider **/
    glm::vec4 landing{ 0.0F };    /**< laid over where a dragged tab would land **/
    float radius = RADIUS;        /**< how far each area's corners are rounded **/
    float line = 1.0F;            /**< how thick an area's line is **/
};

/**
 * A box the frame fills, such as an area's header or its strip of tabs.
 **/
struct Band final {
    glm::vec4 box{ 0.0F };     /**< x, y, width, height in pixels **/
    glm::vec4 colour{ 0.0F };
};

/**
 * Draw the frame around a layout's areas, over the views they show.
 *
 * The gaps are filled, then the bands, which lie across the tops of areas. Each area's corners
 * are rounded by covering what lies outside the rounding in the gap's colour, which rounds a
 * band at an area's corner with it. A line runs just inside each area's edge: a hairline, or the
 * accent for the active area. A lit divider, such as one under the cursor or held, is filled
 * with the accent. Where a dragged tab would land is covered in the landing colour, over the
 * rest. An area with no room draws nothing.
 *
 * @param active the index of the active area, or nothing
 * @param lit the divider to light, or nothing
 * @param landing where a dragged tab would land, or nothing
 * @param bands the boxes filled under the lines
 **/
void drawFrame(v3d::render::realtime::canvas::Canvas* canvas, const AreaLayout& layout, const FrameLook& look,
    std::optional<std::size_t> active, const std::optional<AreaLayout::Divider>& lit,
    const std::optional<glm::vec4>& landing = std::nullopt, std::span<const Band> bands = {});

};  // namespace v3d::editor::view

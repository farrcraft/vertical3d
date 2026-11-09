/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/vec4.hpp>

namespace v3d::editor::view::frame {

/**
 * The colours and sizes the frame around the areas is drawn in.
 **/
struct Look final {
    static constexpr float RADIUS = 4.0F;

    glm::vec4 gap{ 0.0F };        /**< the gaps, and each area's corners outside its rounding **/
    glm::vec4 hairline{ 0.0F };   /**< the line just inside each area's edge **/
    glm::vec4 accent{ 0.0F };     /**< the active area's line, and a lit divider **/
    glm::vec4 landing{ 0.0F };    /**< laid over where a dragged tab would land **/
    float radius = RADIUS;        /**< how far each area's corners are rounded **/
    float line = 1.0F;            /**< how thick an area's line is **/
};

};  // namespace v3d::editor::view::frame

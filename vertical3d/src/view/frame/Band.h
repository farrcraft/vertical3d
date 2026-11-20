/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/vec4.hpp>

namespace v3d::editor::view::frame {

/**
 * A box the frame fills, such as an area's header or its strip of tabs.
 **/
struct Band final {
    glm::vec4 box{ 0.0F };     /**< x, y, width, height in pixels **/
    glm::vec4 colour{ 0.0F };
};

};  // namespace v3d::editor::view::frame

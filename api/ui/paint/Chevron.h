/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/vec2.hpp>

namespace v3d::ui::paint {

/**
 * The three points of a chevron, in canvas pixels, and how thick its strokes are.
 **/
struct Chevron {
    glm::vec2 from{0.0F};
    glm::vec2 tip{0.0F};
    glm::vec2 to{0.0F};
    float width = 0.0F;
};

};  // namespace v3d::ui::paint

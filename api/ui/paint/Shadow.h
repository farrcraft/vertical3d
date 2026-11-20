/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/vec4.hpp>

namespace v3d::ui::paint {

/**
 * How a shadow is drawn: the box's corner radius, how far past the box the fade reaches, and
 * the colour at its darkest. A reach of zero draws the box alone.
 **/
struct Shadow {
    float radius = 0.0F;
    float reach = 0.0F;
    glm::vec4 colour{0.0F};
};

};  // namespace v3d::ui::paint

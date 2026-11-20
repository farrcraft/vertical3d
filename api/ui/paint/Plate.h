/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/vec4.hpp>

namespace v3d::ui::paint {

/**
 * What a box is filled with, and what its outline is drawn in.
 **/
struct Plate {
    glm::vec4 inside{1.0F};
    glm::vec4 outline{1.0F};
};

};  // namespace v3d::ui::paint

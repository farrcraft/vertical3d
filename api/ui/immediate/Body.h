/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/ui/immediate/Id.h>

#include <glm/vec2.hpp>

namespace v3d::ui::immediate {

/**
 * The body of a window or a region: the id that holds its scroll and content, the id its
 * scrollbar is hit tested under, its box, and where the part below any title bar starts.
 **/
struct Body {
    Id id = 0;
    Id scroll = 0;
    glm::vec2 min{0.0F};
    glm::vec2 max{0.0F};
    float top = 0.0F;
};

};  // namespace v3d::ui::immediate

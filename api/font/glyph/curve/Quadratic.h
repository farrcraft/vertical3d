/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/vec2.hpp>

namespace v3d::font::glyph::curve {

/**
 * A quadratic Bézier curve from p1 to p3, pulled towards p2. A straight line has p2 on p3.
 **/
struct Quadratic final {
    glm::vec2 p1{ 0.0F };
    glm::vec2 p2{ 0.0F };
    glm::vec2 p3{ 0.0F };
};

};  // namespace v3d::font::glyph::curve

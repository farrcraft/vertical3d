/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Glyph.h"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::font::glyph::curve {

glm::vec4 Glyph::bandTransform() const {
    const glm::vec2 size = max - min;
    const glm::vec2 counts(static_cast<float>(vertical.size()), static_cast<float>(horizontal.size()));
    const glm::vec2 scale(size.x > 0.0F ? counts.x / size.x : 0.0F, size.y > 0.0F ? counts.y / size.y : 0.0F);
    return glm::vec4(scale, -min * scale);
}

};  // namespace v3d::font::glyph::curve

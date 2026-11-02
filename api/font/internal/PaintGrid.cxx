/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "PaintGrid.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cmath>
#include <cstddef>

#include <glm/vec2.hpp>

namespace v3d::font::internal {

std::size_t PaintGrid::pixels() const {
    return static_cast<std::size_t>(size.x) * static_cast<std::size_t>(size.y);
}

glm::vec2 PaintGrid::em(int column, int row) const {
    return glm::vec2((static_cast<float>(origin.x + column) + 0.5F - shift) / scale, (static_cast<float>(origin.y + size.y - row) - 0.5F) / scale);
}

FT_Vector PaintGrid::device(const glm::vec2& point) const {
    const glm::vec2 at = ((point * scale) + glm::vec2(shift, 0.0F) - glm::vec2(origin)) * FIXED_POINT;
    return FT_Vector{ .x = static_cast<FT_Pos>(std::lround(at.x)), .y = static_cast<FT_Pos>(std::lround(at.y)) };
}

};  // namespace v3d::font::internal

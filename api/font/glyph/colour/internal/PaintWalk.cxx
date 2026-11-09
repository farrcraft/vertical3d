/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "PaintWalk.h"

#include <api/font/glyph/colour/Fill.h>
#include <api/font/glyph/colour/Layer.h>

#include <utility>

#include <glm/common.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>

namespace v3d::font::glyph::colour::internal {

void PaintWalk::Box::hold(const glm::vec2& point) {
    min = any ? glm::min(min, point) : point;
    max = any ? glm::max(max, point) : point;
    any = true;
}

glm::mat3 PaintWalk::current() const {
    return toEm * transforms.back();
}

void PaintWalk::fill(Fill&& made) {
    Layer layer{ .kind = Layer::Kind::Fill, .clips = clips };
    layer.fill = std::move(made);
    glyph.layers.push_back(std::move(layer));
}

};  // namespace v3d::font::glyph::colour::internal

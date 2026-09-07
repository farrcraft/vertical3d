/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Painter.h"

#include <algorithm>

namespace v3d::ui {

namespace {

/**
 * How many segments one rounded corner is approximated with. A corner is a small arc and a
 * handful of triangles is enough of one; the cost is per box per frame.
 **/
const unsigned int cornerSides = 6;

/**
 * A quarter turn, which is what each corner of a rounded box sweeps.
 **/
const float quarterTurn = 1.5707963267948966f;

};  // namespace

void fillBox(v3d::render::realtime::Canvas* canvas, const glm::vec2& min, const glm::vec2& max,
    float radius, const glm::vec4& colour) {
    const glm::vec2 size = max - min;
    if (canvas == nullptr || size.x <= 0.0f || size.y <= 0.0f || colour.a <= 0.0f) {
        return;
    }
    // a radius past half the shorter side would fold the box over itself
    const float corner = std::min(radius, std::min(size.x, size.y) * 0.5f);
    if (corner <= 0.0f) {
        canvas->rect(min, max, colour);
        return;
    }

    // three bands and four wedges, none of them overlapping - which matters because a box
    // is usually drawn with an alpha, and anything drawn twice under one would show
    canvas->rect(glm::vec2(min.x + corner, min.y), glm::vec2(max.x - corner, max.y), colour);
    canvas->rect(glm::vec2(min.x, min.y + corner), glm::vec2(min.x + corner, max.y - corner), colour);
    canvas->rect(glm::vec2(max.x - corner, min.y + corner), glm::vec2(max.x, max.y - corner), colour);

    canvas->arc(glm::vec2(min.x + corner, min.y + corner), corner, cornerSides, quarterTurn * 2.0f, quarterTurn, colour);
    canvas->arc(glm::vec2(max.x - corner, min.y + corner), corner, cornerSides, quarterTurn * 3.0f, quarterTurn, colour);
    canvas->arc(glm::vec2(max.x - corner, max.y - corner), corner, cornerSides, 0.0f, quarterTurn, colour);
    canvas->arc(glm::vec2(min.x + corner, max.y - corner), corner, cornerSides, quarterTurn, quarterTurn, colour);
}

void plateBox(v3d::render::realtime::Canvas* canvas, const glm::vec2& min, const glm::vec2& max,
    float radius, float width, const glm::vec4& inside, const glm::vec4& outline) {
    if (width <= 0.0f || outline.a <= 0.0f) {
        fillBox(canvas, min, max, radius, inside);
        return;
    }
    fillBox(canvas, min, max, radius, outline);
    const glm::vec2 inset(width, width);
    fillBox(canvas, min + inset, max - inset, std::max(0.0f, radius - width), inside);
}

bool inside(const glm::vec2& min, const glm::vec2& max, const glm::vec2& point) noexcept {
    return point.x >= min.x && point.y >= min.y && point.x < max.x && point.y < max.y;
}

};  // namespace v3d::ui

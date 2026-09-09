/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Painter.h"

#include <algorithm>

namespace v3d::ui::paint {

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

/**
 * A rectangle between two corners in any order, and nothing at all when they make no box.
 **/
void box(v3d::render::realtime::Canvas* canvas, const glm::vec2& first, const glm::vec2& second,
    const glm::vec4& colour) {
    const glm::vec2 low(std::min(first.x, second.x), std::min(first.y, second.y));
    const glm::vec2 high(std::max(first.x, second.x), std::max(first.y, second.y));
    if (high.x > low.x && high.y > low.y) {
        canvas->rect(low, high, colour);
    }
}

/**
 * One corner of an outline: the band that turns through it, and the square left over when
 * the outline is thicker than the corner is round.
 *
 * @param corner the box's corner, which the square runs inwards from
 * @param towards which way inwards is, each component 1 or -1
 * @param start where the quarter turn begins, as arc() takes it
 **/
void turn(v3d::render::realtime::Canvas* canvas, const glm::vec2& corner, const glm::vec2& towards,
    float radius, float width, float start, const glm::vec4& colour) {
    canvas->ring(corner + towards * radius, radius, radius - width, cornerSides, start, quarterTurn, colour);
    // the turn fills a square of the radius; an outline thicker than that leaves the rest
    // of a square of its own width, which is two rectangles beside the turn
    if (width <= radius) {
        return;
    }
    box(canvas, corner + glm::vec2(towards.x * radius, 0.0f), corner + towards * width, colour);
    box(canvas, corner + glm::vec2(0.0f, towards.y * radius),
        corner + glm::vec2(towards.x * radius, towards.y * width), colour);
}

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

void strokeBox(v3d::render::realtime::Canvas* canvas, const glm::vec2& min, const glm::vec2& max,
    float radius, float width, const glm::vec4& colour) {
    const glm::vec2 size = max - min;
    if (canvas == nullptr || size.x <= 0.0f || size.y <= 0.0f || width <= 0.0f || colour.a <= 0.0f) {
        return;
    }
    // an outline half the shorter side thick is the whole box, and neither it nor the
    // radius may pass that without folding the box over itself
    const float half = std::min(size.x, size.y) * 0.5f;
    const float corner = std::min(radius, half);
    const float thickness = std::min(width, half);

    if (corner <= 0.0f) {
        // a square outline is four runs, with each corner taken by the run above or below
        // it rather than by a turn of its own
        box(canvas, min, glm::vec2(max.x, min.y + thickness), colour);
        box(canvas, glm::vec2(min.x, max.y - thickness), max, colour);
        box(canvas, glm::vec2(min.x, min.y + thickness), glm::vec2(min.x + thickness, max.y - thickness), colour);
        box(canvas, glm::vec2(max.x - thickness, min.y + thickness), glm::vec2(max.x, max.y - thickness), colour);
        return;
    }

    // how far in from each corner the straight runs start, which is the corner's own
    // square or the outline's width where that is wider
    const float taken = std::max(corner, thickness);

    box(canvas, glm::vec2(min.x + taken, min.y), glm::vec2(max.x - taken, min.y + thickness), colour);
    box(canvas, glm::vec2(min.x + taken, max.y - thickness), glm::vec2(max.x - taken, max.y), colour);
    box(canvas, glm::vec2(min.x, min.y + taken), glm::vec2(min.x + thickness, max.y - taken), colour);
    box(canvas, glm::vec2(max.x - thickness, min.y + taken), glm::vec2(max.x, max.y - taken), colour);

    turn(canvas, min, glm::vec2(1.0f, 1.0f), corner, thickness, quarterTurn * 2.0f, colour);
    turn(canvas, glm::vec2(max.x, min.y), glm::vec2(-1.0f, 1.0f), corner, thickness, quarterTurn * 3.0f, colour);
    turn(canvas, max, glm::vec2(-1.0f, -1.0f), corner, thickness, 0.0f, colour);
    turn(canvas, glm::vec2(min.x, max.y), glm::vec2(1.0f, -1.0f), corner, thickness, quarterTurn, colour);
}

void plateBox(v3d::render::realtime::Canvas* canvas, const glm::vec2& min, const glm::vec2& max,
    float radius, float width, const glm::vec4& inside, const glm::vec4& outline) {
    if (width <= 0.0f || outline.a <= 0.0f) {
        fillBox(canvas, min, max, radius, inside);
        return;
    }
    strokeBox(canvas, min, max, radius, width, outline);
    const glm::vec2 inset(width, width);
    fillBox(canvas, min + inset, max - inset, std::max(0.0f, radius - width), inside);
}

bool inside(const glm::vec2& min, const glm::vec2& max, const glm::vec2& point) noexcept {
    return point.x >= min.x && point.y >= min.y && point.x < max.x && point.y < max.y;
}

};  // namespace v3d::ui::paint

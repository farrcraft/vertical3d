/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../render/realtime/Canvas.h"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui {

/**
 * The boxes this library is drawn out of, shared by the two ways of writing a ui: the
 * retained components of ADR-0034 and the immediate mode layer of ADR-0035.
 *
 * A rounded box is three bands and four wedges, all of them the one batched primitive of
 * ADR-0005, so a corner radius costs no draw of its own.
 **/

/**
 * Fill a box, with its corners rounded by a radius. A radius of zero is one quad.
 *
 * A radius past half the shorter side is clamped to it, which is what makes a fully
 * rounded end rather than a folded one.
 **/
void fillBox(v3d::render::realtime::Canvas* canvas, const glm::vec2& min, const glm::vec2& max,
    float radius, const glm::vec4& colour);

/**
 * Fill a box inside an outline, both rounded.
 *
 * The outline is the same box drawn behind rather than four edges around it, so a rounded
 * corner needs no second shape to trace it.
 *
 * @param width how thick the outline is; nothing is drawn behind when it is zero
 **/
void plateBox(v3d::render::realtime::Canvas* canvas, const glm::vec2& min, const glm::vec2& max,
    float radius, float width, const glm::vec4& inside, const glm::vec4& outline);

/**
 * @return whether a point is inside a box, its top and left edges included
 **/
bool inside(const glm::vec2& min, const glm::vec2& max, const glm::vec2& point) noexcept;

};  // namespace v3d::ui

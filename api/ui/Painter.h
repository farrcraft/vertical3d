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
 * Trace a box's edges, with its corners rounded by a radius.
 *
 * Four straight runs and four bands turning between them, none of them covering anything
 * inside the outline. An outline thicker than the radius it turns squares its corners off
 * rather than folding them.
 *
 * @param width how thick the outline is, clamped to half the shorter side
 **/
void strokeBox(v3d::render::realtime::Canvas* canvas, const glm::vec2& min, const glm::vec2& max,
    float radius, float width, const glm::vec4& colour);

/**
 * Fill a box inside an outline, both rounded.
 *
 * The outline is traced around the fill rather than drawn as a box behind it, because
 * what is inside usually carries an alpha: an outline behind it would show through as a
 * tint over the whole plate, and whatever the plate covers would not show through at all.
 *
 * @param width how thick the outline is; nothing is traced when it is zero
 **/
void plateBox(v3d::render::realtime::Canvas* canvas, const glm::vec2& min, const glm::vec2& max,
    float radius, float width, const glm::vec4& inside, const glm::vec4& outline);

/**
 * @return whether a point is inside a box, its top and left edges included
 **/
bool inside(const glm::vec2& min, const glm::vec2& max, const glm::vec2& point) noexcept;

};  // namespace v3d::ui

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/canvas/Canvas.h>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::paint {

/**
 * The boxes this library is drawn out of, shared by the retained components and the
 * immediate ui.
 *
 * A rounded box is three bands and four wedges, all of them batched on the 2D canvas, so a
 * corner radius costs no draw of its own.
 *
 * A curved or slanted edge is smoothed: it fades to nothing across a pixel centred on it, so a
 * small radius still reads as round. A straight edge along an axis is left hard.
 **/

/**
 * Fill a box, with its corners rounded by a radius. A radius of zero is one quad.
 *
 * A radius past half the shorter side is clamped to it, so it makes a fully rounded
 * end rather than a folded one. A rounded corner reaches half a pixel past its curve as it
 * fades.
 **/
void fillBox(v3d::render::realtime::canvas::Canvas* canvas, const glm::vec2& min, const glm::vec2& max,
    float radius, const glm::vec4& colour);

/**
 * Trace a box's edges, with its corners rounded by a radius.
 *
 * Four straight runs and four bands turning between them, none of them covering anything
 * inside the outline. An outline thicker than the radius it turns squares its corners off
 * rather than folding them. A turn's curved edges fade over half a pixel either side, so it
 * reaches that far past the outline's bounds, inwards and outwards.
 *
 * @param canvas what the outline is drawn on, or null to draw nothing
 * @param min the box's top left corner, in canvas pixels
 * @param max the box's bottom right corner, in canvas pixels
 * @param radius how far the corners are rounded, clamped to half the shorter side
 * @param width how thick the outline is, clamped to half the shorter side
 * @param colour what the outline is drawn in. A transparent one draws nothing
 **/
void strokeBox(v3d::render::realtime::canvas::Canvas* canvas, const glm::vec2& min, const glm::vec2& max,
    float radius, float width, const glm::vec4& colour);

/**
 * What a box is filled with, and what its outline is drawn in.
 **/
struct Plate {
    glm::vec4 inside{1.0F};
    glm::vec4 outline{1.0F};
};

/**
 * Fill a box inside an outline, both rounded.
 *
 * The outline is traced around the fill, not drawn as a box behind it, because the fill
 * usually carries an alpha. An outline behind it would tint the whole box, and would hide
 * whatever the box covers.
 *
 * @param canvas what the box is drawn on, or null to draw nothing
 * @param min the box's top left corner, in canvas pixels
 * @param max the box's bottom right corner, in canvas pixels
 * @param radius how far the outline's corners are rounded. The fill's corners are rounded by
 *        the radius less the width
 * @param width how thick the outline is. Nothing is traced when it is zero
 * @param colours what the box is filled with, and what its outline is
 **/
void plateBox(v3d::render::realtime::canvas::Canvas* canvas, const glm::vec2& min, const glm::vec2& max,
    float radius, float width, const Plate& colours);

/**
 * How a shadow is drawn: the box's corner radius, how far past the box the fade reaches, and
 * the colour at its darkest. A reach of zero draws the box alone.
 **/
struct Shadow {
    float radius = 0.0F;
    float reach = 0.0F;
    glm::vec4 colour{0.0F};
};

/**
 * A shadow: a rounded box filled with a colour, with an edge that fades out to nothing.
 *
 * The fade follows half a turn of a cosine, drawn as a few bands that each blend between two
 * points on it, so it has no hard inner edge as a straight ramp would. Each corner turns about
 * the same centre as the box's own, so the fade rounds even a square box's corners.
 *
 * Draw it before the box that casts it. Its inside is filled, so a box drawn over it with an
 * alpha shows the shadow through. Its corners are not smoothed, since the fade already softens
 * them.
 **/
void shadowBox(v3d::render::realtime::canvas::Canvas* canvas, const glm::vec2& min, const glm::vec2& max,
    const Shadow& shadow);

/**
 * Draw a drop mark: a chevron pointing down, which says a click opens a list or a menu.
 *
 * @param canvas what the mark is drawn on, or null to draw nothing
 * @param centre the middle of the mark, in canvas pixels
 * @param width how wide the mark is. Its strokes are a quarter of that, never under a pixel, and
 *        smoothed along their sides
 * @param colour what the mark is drawn in
 **/
void dropMark(v3d::render::realtime::canvas::Canvas* canvas, const glm::vec2& centre, float width, const glm::vec4& colour);

/**
 * The three points of a chevron, in canvas pixels, and how thick its strokes are.
 **/
struct Chevron {
    glm::vec2 from{0.0F};
    glm::vec2 tip{0.0F};
    glm::vec2 to{0.0F};
    float width = 0.0F;
};

/**
 * Draw a chevron: a stroke from its start to its tip and another on to its end, each smoothed
 * along its sides. Each stroke runs on past the tip until the outer edges meet, so the bend
 * comes to a point rather than leaving a notch. The strokes overlap there, so a translucent
 * colour shows the overlap.
 *
 * @param canvas what the chevron is drawn on, or null to draw nothing
 **/
void chevron(v3d::render::realtime::canvas::Canvas* canvas, const Chevron& shape, const glm::vec4& colour);

/**
 * @return whether a point is inside a box, its top and left edges included
 **/
bool inside(const glm::vec2& min, const glm::vec2& max, const glm::vec2& point) noexcept;

};  // namespace v3d::ui::paint

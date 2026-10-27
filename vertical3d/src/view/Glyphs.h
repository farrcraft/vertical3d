/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/curve/Curves.h>
#include <api/render/realtime/canvas/Line.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Light.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * How far a glyph reaches along its item's +z axis, in world units. A line canvas draws in
 * world space, so a glyph has a size in the scene rather than on the screen.
 **/
constexpr float GLYPH_LENGTH = 1.0F;

/**
 * The width of a camera glyph's picture over its height. A camera does not hold the shape of
 * its picture, which a render's settings give, so the glyph draws a common one.
 **/
constexpr float GLYPH_ASPECT = 4.0F / 3.0F;

/**
 * How far a group's box reaches from its origin along each axis, in world units.
 **/
constexpr float GROUP_REACH = 0.25F;

/**
 * Where a glyph is drawn: where its item's world matrix puts it, without the scale. A camera
 * and a light are not scaled, and a group is placed by its scale but drawn without it, so a
 * glyph keeps its size whatever scale it inherits.
 **/
glm::mat4 glyphPlacement(const glm::mat4& world);

/**
 * A camera, in its own space, looking along +z with +y up.
 *
 * A perspective camera is a pyramid from its eye to a picture GLYPH_LENGTH away, as wide as its
 * angle of view. An orthographic camera is a box as tall as twice its extent, GLYPH_LENGTH deep.
 * A triangle above the far picture points up.
 **/
void drawCamera(const Camera::Lens& lens, const glm::vec4& colour, v3d::render::realtime::canvas::Line* canvas);

/**
 * A light, in its own space.
 *
 * A point light is a star of three lines through its position. A distant light is three
 * parallel lines along +z, the middle one with an arrowhead. A spot light is a cone from its
 * position along +z, GLYPH_LENGTH along each side, with a ring at its edge and a second ring
 * where its soft edge begins.
 **/
void drawLight(const Light::Emission& emission, const glm::vec4& colour, v3d::render::realtime::canvas::Line* canvas);

/**
 * A group, in its own space: a box GROUP_REACH from its origin along each axis.
 **/
void drawGroup(const glm::vec4& colour, v3d::render::realtime::canvas::Line* canvas);

/**
 * Curves, in their own space: a line between each pair of evaluated points of each curve.
 **/
void drawCurves(const v3d::curve::Curves& curves, const glm::vec4& colour, v3d::render::realtime::canvas::Line* canvas);

};  // namespace v3d::editor

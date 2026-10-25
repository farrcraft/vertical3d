/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/canvas/Line.h>

#include "Camera.h"
#include "Item.h"
#include "Light.h"

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
 * Where a camera's or a light's glyph is drawn: the item's translation and rotation. Its scale
 * is left out, because neither kind is scaled, so a glyph keeps its size whatever the scale.
 **/
glm::mat4 glyphPlacement(const Item& item);

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

};  // namespace v3d::editor

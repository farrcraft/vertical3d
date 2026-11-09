/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/render/realtime/canvas/Canvas.h>
#include <api/type/camera/Camera.h>
#include <api/ui/paint/Type.h>

#include <cstddef>
#include <optional>
#include <vector>

#include "AxisEnd.h"
#include "OverlayLook.h"
#include "OverlayMetrics.h"
#include "ViewPort.h"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::view {

/**
 * The ends of the three world axes as a camera sees them, about a centre on the screen.
 *
 * Each end lies a length from the centre, in the direction the axis runs on the screen. The
 * axes are drawn as an orthographic camera would show them, so a perspective camera's
 * foreshortening does not bend them.
 *
 * @param negative whether to give the negative ends as well
 * @return the ends, farthest from the viewer first, so drawing them in order puts the nearest
 *         on top
 **/
std::vector<AxisEnd> axisEnds(const v3d::type::camera::Camera& camera, const glm::vec2& centre, float length, bool negative);

/**
 * @return where the axis marker's centre is in a view's region
 **/
glm::vec2 markerCentre(const glm::vec4& region, const OverlayMetrics& metrics);

/**
 * @return where the navigation gizmo's centre is in a view's region
 **/
glm::vec2 gizmoCentre(const glm::vec4& region, const OverlayMetrics& metrics);

/**
 * @return whether a view draws a navigation gizmo, which a perspective view does
 **/
bool hasGizmo(const ViewPort& view);

/**
 * The ball of a view's gizmo at a point. Where two balls overlap, the one nearer the viewer is
 * found.
 *
 * @return the end of the axis the ball is on, or nothing when no ball is there or the view has
 *         no gizmo
 **/
std::optional<AxisEnd> gizmoAt(const ViewPort& view, const OverlayMetrics& metrics, const glm::vec2& point);

/**
 * Turn a camera to look down an axis from its end, at the point it looks at.
 *
 * The point is where the camera's line of sight passes nearest the world's origin, or a unit in
 * front of the eye when that is behind it. The eye keeps its distance from that point. A camera
 * looking down the y axis has z for up, as the Top and Bottom views do, and any other has y.
 **/
void lookAlong(v3d::type::camera::Camera* camera, const AxisEnd& end);

/**
 * Draw a view's axis marker, and its gizmo when it has one, in window pixels.
 *
 * The marker is a line from its centre along each axis, in the axis's colour, with the axis's
 * letter beyond the line's end. An axis seen end on, whose line is shorter than a quarter of
 * the marker's, has no letter. The gizmo is a ball at each end of each axis: a filled one with
 * its letter at a positive end, and a ring at a negative one. A line joins the centre to each
 * positive ball. A faint disc lies behind the gizmo, brighter while the cursor is over it, and a
 * ring goes around the ball under the cursor.
 *
 * @param cursor where the cursor is, or nothing when it is not over the view
 **/
void drawOverlay(v3d::render::realtime::canvas::Canvas* canvas, const ViewPort& view, const OverlayMetrics& metrics,
    const OverlayLook& look, const v3d::ui::paint::Type& type,
    const std::optional<glm::vec2>& cursor);

};  // namespace v3d::editor::view

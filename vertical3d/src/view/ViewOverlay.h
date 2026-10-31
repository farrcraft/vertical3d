/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/canvas/Canvas.h>
#include <api/type/camera/Camera.h>
#include <api/ui/paint/Text.h>

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

#include "ViewPort.h"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * Where a view's axis marker and navigation gizmo sit, and how large they are, in pixels.
 *
 * The marker sits in the view's lower left corner. The gizmo sits in the upper right corner,
 * under the area's header, and only a perspective view has one.
 **/
struct OverlayMetrics final {
    static constexpr float MARKER = 20.0F;
    static constexpr float INSET = 36.0F;
    static constexpr float GIZMO = 30.0F;
    static constexpr float BALL = 9.0F;
    static constexpr float MARGIN = 8.0F;

    float marker = MARKER;   /**< how long each line of the marker is **/
    float inset = INSET;     /**< how far the marker's centre is from the view's left and bottom edges **/
    float gizmo = GIZMO;     /**< how far each of the gizmo's balls is from its centre **/
    float ball = BALL;       /**< the radius of a ball **/
    float margin = MARGIN;   /**< between the gizmo's balls and the view's right edge or the header **/
    float top = 0.0F;        /**< how much of the top of the view the header covers **/

    /**
     * @return the metrics at a ui scale, under a header of a height already at that scale
     **/
    static OverlayMetrics scaled(float factor, float top);
};

/**
 * One end of a world axis, as the marker or the gizmo draws it.
 **/
struct AxisEnd final {
    std::size_t axis = 0;      /**< 0, 1 or 2 for x, y and z **/
    bool positive = true;      /**< whether this is the axis's positive end **/
    glm::vec2 point{ 0.0F };   /**< where it is drawn, in window pixels **/
    float depth = 0.0F;        /**< how far it points toward the viewer, from -1 to 1 **/
};

/**
 * The colours the overlays are drawn in.
 **/
struct OverlayLook final {
    static constexpr glm::vec4 X_AXIS{ 0.851F, 0.349F, 0.373F, 1.0F };
    static constexpr glm::vec4 Y_AXIS{ 0.447F, 0.690F, 0.322F, 1.0F };
    static constexpr glm::vec4 Z_AXIS{ 0.333F, 0.565F, 0.910F, 1.0F };
    static constexpr glm::vec4 LABEL{ 0.067F, 0.071F, 0.078F, 1.0F };
    static constexpr glm::vec4 HOVER{ 1.0F, 1.0F, 1.0F, 0.08F };
    static constexpr float TEXT = 11.0F;
    static constexpr float LINE = 2.0F;

    std::array<glm::vec4, 3> axes{ X_AXIS, Y_AXIS, Z_AXIS };   /**< x, y and z **/
    glm::vec4 label = LABEL;                                  /**< a letter on a ball **/
    glm::vec4 hover = HOVER;                                  /**< the disc behind the gizmo under the cursor **/
    glm::vec4 lit{ 1.0F };                                    /**< the ring around the ball under the cursor **/
    float text = TEXT;                                        /**< how large a letter is drawn, in pixels **/
    float line = LINE;                                        /**< how thick a line is, and a ring around a ball **/
};

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
 * positive ball. A disc lies behind the gizmo while the cursor is over it, and a ring around the
 * ball under the cursor.
 *
 * @param cursor where the cursor is, or nothing when it is not over the view
 **/
void drawOverlay(v3d::render::realtime::canvas::Canvas* canvas, const ViewPort& view, const OverlayMetrics& metrics,
    const OverlayLook& look, const v3d::ui::paint::Measure& measure, const v3d::ui::paint::Write& write,
    const std::optional<glm::vec2>& cursor);

};  // namespace v3d::editor

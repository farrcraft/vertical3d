/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::editor::view {

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

};  // namespace v3d::editor::view

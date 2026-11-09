/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "OverlayMetrics.h"

namespace v3d::editor::view {

/**
 **/
OverlayMetrics OverlayMetrics::scaled(float factor, float top) {
    return OverlayMetrics{ .marker = MARKER * factor, .inset = INSET * factor, .gizmo = GIZMO * factor, .ball = BALL * factor,
        .margin = MARGIN * factor, .top = top };
}

};  // namespace v3d::editor::view

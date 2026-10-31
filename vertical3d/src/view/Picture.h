/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::editor::view {

/**
 * A camera glyph's picture: a rectangle about the +z axis, at a distance along it.
 **/
struct Picture {
    float halfWidth = 0.0F;
    float halfHeight = 0.0F;
    float depth = 0.0F;
};

};  // namespace v3d::editor::view

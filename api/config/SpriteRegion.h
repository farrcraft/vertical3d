/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::config {

/**
 * Where one sprite sits in the sheet that holds it, in the sheet's own pixels.
 **/
struct SpriteRegion final {
    SpriteRegion() noexcept;

    int x;
    int y;
    int width;
    int height;
};

};  // namespace v3d::config

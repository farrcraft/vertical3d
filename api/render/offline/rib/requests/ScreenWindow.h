/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::render::offline::rib::requests {

/**
 * RiScreenWindow: the screen space rectangle the image covers.
 **/
struct ScreenWindow {
    float left = -1.0F;
    float right = 1.0F;
    float bottom = -1.0F;
    float top = 1.0F;
};

};  // namespace v3d::render::offline::rib::requests

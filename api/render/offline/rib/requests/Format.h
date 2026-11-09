/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::render::offline::rib::requests {

/**
 * RiFormat. RI reads a side of zero or less as the renderer's default for that side, and such
 * a side arrives as 0. Every other side is from 1 to largestResolution.
 **/
struct Format {
    unsigned int width = 0;
    unsigned int height = 0;
    float pixelAspect = 1.0F;
};

};  // namespace v3d::render::offline::rib::requests

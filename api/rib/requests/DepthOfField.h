/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::rib::requests {

/**
 * RiDepthOfField. An infinite fstop is a pinhole camera.
 **/
struct DepthOfField {
    float fstop = 0.0F;
    float focalLength = 0.0F;
    float focalDistance = 0.0F;
};

};  // namespace v3d::rib::requests

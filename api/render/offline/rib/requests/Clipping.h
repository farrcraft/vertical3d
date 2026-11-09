/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::render::offline::rib::requests {

/**
 * RiClipping: the camera space depths of the near and far clipping planes.
 **/
struct Clipping {
    float hither = 0.0F;
    float yon = 0.0F;
};

};  // namespace v3d::render::offline::rib::requests

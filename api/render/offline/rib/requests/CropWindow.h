/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::render::offline::rib::requests {

/**
 * RiCropWindow: the part of the image to render, as fractions of it.
 **/
struct CropWindow {
    float xmin = 0.0F;
    float xmax = 1.0F;
    float ymin = 0.0F;
    float ymax = 1.0F;
};

};  // namespace v3d::render::offline::rib::requests

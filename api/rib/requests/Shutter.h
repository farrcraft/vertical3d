/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::rib::requests {

/**
 * RiShutter: when the shutter opens and closes.
 **/
struct Shutter {
    float open = 0.0F;
    float close = 0.0F;
};

};  // namespace v3d::rib::requests

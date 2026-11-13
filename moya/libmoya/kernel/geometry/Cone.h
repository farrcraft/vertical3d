/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>

namespace v3d::moya::kernel::geometry {

/**
 * A ray's cone: how wide the area a camera sample covers is where the ray starts, and how much
 * wider it grows over each unit of distance along the ray, in world units. A spread below zero
 * narrows the cone, as a lens's does towards its focus, and the cone is as wide on either side
 * of where it narrows to nothing.
 **/
struct Cone {
    float width { 0.0F };
    float spread { 0.0F };

    /** Its width a distance along the ray, in world units. **/
    MOYA_KERNEL float widthAt(float distance) const {
        return std::fabs(width + (spread * distance));
    }
};

};  // namespace v3d::moya::kernel::geometry

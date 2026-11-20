/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include "Sphere.h"

namespace v3d::rib::requests {

/** RiDisk: a disk about z at a height, swept through an angle in degrees. **/
struct Disk {
    float height = 0.0F;
    float radius = 1.0F;
    float thetamax = Sphere::FULL_THETAMAX;
};

};  // namespace v3d::rib::requests

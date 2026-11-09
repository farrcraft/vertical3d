/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include "Sphere.h"

namespace v3d::render::offline::rib::requests {

/** RiCylinder: a cylinder about z between two heights, swept through an angle in degrees. **/
struct Cylinder {
    float radius = 1.0F;
    float zmin = -1.0F;
    float zmax = 1.0F;
    float thetamax = Sphere::FULL_THETAMAX;
};

};  // namespace v3d::render::offline::rib::requests

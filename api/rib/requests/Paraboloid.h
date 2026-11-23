/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include "Sphere.h"

namespace v3d::rib::requests {

/**
 * RiParaboloid: a paraboloid about z, of radius rmax at zmax, cut at two heights and swept
 * through an angle in degrees.
 **/
struct Paraboloid {
    float rmax = 1.0F;
    float zmin = 0.0F;
    float zmax = 1.0F;
    float thetamax = Sphere::FULL_THETAMAX;
};

};  // namespace v3d::rib::requests

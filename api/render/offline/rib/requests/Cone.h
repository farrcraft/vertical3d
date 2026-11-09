/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include "Sphere.h"

namespace v3d::render::offline::rib::requests {

/**
 * RiCone: a cone about z, its base at the origin and its apex at a height, swept through an angle
 * in degrees.
 **/
struct Cone {
    float height = 1.0F;
    float radius = 1.0F;
    float thetamax = Sphere::FULL_THETAMAX;
};

};  // namespace v3d::render::offline::rib::requests

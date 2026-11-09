/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include "Sphere.h"

namespace v3d::render::offline::rib::requests {

/**
 * RiTorus: a circle of the minor radius, centred at the major radius from z, swept from phimin to
 * phimax about its own centre and through thetamax about z, all in degrees.
 **/
struct Torus {
    /** RI gives a torus no default. This is a ring a quarter as thick as it is wide. **/
    static constexpr float DEFAULT_MINOR_RADIUS = 0.25F;

    float majorRadius = 1.0F;
    float minorRadius = DEFAULT_MINOR_RADIUS;
    float phimin = 0.0F;
    float phimax = Sphere::FULL_THETAMAX;
    float thetamax = Sphere::FULL_THETAMAX;
};

};  // namespace v3d::render::offline::rib::requests

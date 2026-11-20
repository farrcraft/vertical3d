/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <array>

#include "Sphere.h"

namespace v3d::rib::requests {

/** RiHyperboloid: the line between two points, swept about z through an angle in degrees. **/
struct Hyperboloid {
    std::array<float, 3> point1 { 1.0F, 0.0F, -1.0F };
    std::array<float, 3> point2 { 1.0F, 0.0F, 1.0F };
    float thetamax = Sphere::FULL_THETAMAX;
};

};  // namespace v3d::rib::requests

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::rib::requests {

/**
 * RiSphere: a sphere about the origin, cut at two heights and swept through an angle in
 * degrees.
 **/
struct Sphere {
    /** The thetamax of a whole turn, which closes the sphere. **/
    static constexpr float FULL_THETAMAX = 360.0F;

    float radius = 1.0F;
    float zmin = -1.0F;
    float zmax = 1.0F;
    float thetamax = FULL_THETAMAX;
};

};  // namespace v3d::rib::requests

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/integrate/Profile.h>

namespace v3d::moya::bench {

/**
 * What the integrator gives over a scene at one instruction set: the millions of samples and of
 * rays a second, the rays each sample takes, and the counted render's profile.
 **/
struct Measured {
    double samples { 0.0 };
    double rays { 0.0 };
    double perSample { 0.0 };
    integrate::Profile counted;
};

};  // namespace v3d::moya::bench

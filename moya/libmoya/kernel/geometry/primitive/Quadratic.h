/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::moya::kernel::geometry::primitive {

/** The coefficients of squared t^2 + linear t + constant. **/
struct Quadratic {
    float squared { 0.0F };
    float linear { 0.0F };
    float constant { 0.0F };
};

};  // namespace v3d::moya::kernel::geometry::primitive

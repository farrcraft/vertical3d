/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::moya::film {

/** A luminance estimated from samples, and its standard error. **/
struct Estimate {
    float value { 0.0F };
    float error { 0.0F };
};

};  // namespace v3d::moya::film

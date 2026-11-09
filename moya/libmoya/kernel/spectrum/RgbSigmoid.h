/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>

namespace v3d::moya::kernel::spectrum {

/**
 * A smooth spectrum from three coefficients: sigmoid(c0 lambda^2 + c1 lambda + c2), with lambda
 * in nanometres and sigmoid(x) = 1/2 + x / (2 sqrt(1 + x^2)). It lies in (0, 1) at every
 * wavelength, so it is a reflectance a surface can have. Jakob and Hanika's method finds one for
 * an RGB colour.
 **/
struct RgbSigmoid {
    float c0 { 0.0F };
    float c1 { 0.0F };
    float c2 { 0.0F };

    /** The spectrum's value at a wavelength in nanometres. **/
    MOYA_KERNEL float operator()(float lambda) const {
        const float x = (((c0 * lambda) + c1) * lambda) + c2;
        if (std::isinf(x)) {
            return x > 0.0F ? 1.0F : 0.0F;
        }
        return 0.5F + (x / (2.0F * std::sqrt(1.0F + (x * x))));
    }
};

};  // namespace v3d::moya::kernel::spectrum

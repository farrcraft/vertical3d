/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Lobe.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::bsdf {

/**
 * A direction a BSDF sampled: the direction light comes in from, in world space, the BSDF's value
 * for it, the density it was drawn with, and the kind of bounce. A density of zero is no sample.
 **/
struct Sample {
    glm::vec3 incoming { 0.0F, 0.0F, 1.0F };
    spectrum::SampledSpectrum value { 0.0F };
    float density { 0.0F };
    Lobe lobe { Lobe::Diffuse };

    MOYA_KERNEL bool found() const {
        return density > 0.0F;
    }
};

};  // namespace v3d::moya::kernel::bsdf

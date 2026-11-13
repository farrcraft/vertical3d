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
 *
 * A specular sample was drawn from a closure that scatters into a single direction. Its value is
 * the share of light that closure carries along incoming, with no cosine, and its density is the
 * chance the closure was picked. No other strategy can draw its direction, so nothing weighs it
 * against one.
 *
 * Its roughness is that of the closure it was drawn from, which widens a ray's cone. It is a
 * microfacet lobe's GGX alpha, or the geometric mean of an anisotropic lobe's two, one for a
 * diffuse lobe, and zero for a specular sample.
 **/
struct Sample {
    glm::vec3 incoming { 0.0F, 0.0F, 1.0F };
    spectrum::SampledSpectrum value { 0.0F };
    float density { 0.0F };
    Lobe lobe { Lobe::Diffuse };
    float roughness { 0.0F };
    bool specular { false };

    MOYA_KERNEL bool found() const {
        return density > 0.0F;
    }
};

};  // namespace v3d::moya::kernel::bsdf

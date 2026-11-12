/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/F82.h>
#include <moya/libmoya/kernel/bsdf/Ggx.h>
#include <moya/libmoya/kernel/bsdf/Lobe.h>
#include <moya/libmoya/kernel/bsdf/Model.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>

#include <glm/vec2.hpp>

namespace v3d::moya::kernel::bsdf {

/**
 * One lobe of a BSDF: its model, its weight at the path's wavelengths, the estimate of its albedo
 * that sampling picks it by, and what its model reads. A microfacet model reads its alphas along
 * the tangent and the bitangent, and a conductor its Fresnel factor.
 **/
struct Closure {
    Model model { Model::Diffuse };
    spectrum::SampledSpectrum weight { 0.0F };
    float albedo { 0.0F };
    glm::vec2 alpha { 0.5F, 0.5F };
    F82 fresnel;

    /**
     * Whether it scatters into a single direction for each outgoing one: a mirror, or a
     * microfacet lobe too smooth to have microfacets. Such a closure has no value and no density
     * for any direction it is given, and only its sample finds its light.
     **/
    MOYA_KERNEL bool singular() const {
        return model == Model::Mirror || (model == Model::Conductor && Ggx::smooth(alpha));
    }

    /** The kind of bounce its sample makes. **/
    MOYA_KERNEL Lobe lobe() const {
        if (singular()) {
            return Lobe::Specular;
        }
        return model == Model::Conductor ? Lobe::Glossy : Lobe::Diffuse;
    }
};

};  // namespace v3d::moya::kernel::bsdf

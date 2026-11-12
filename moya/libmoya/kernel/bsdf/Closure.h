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

/** How far from one a dielectric's index lies before it bends light at all. **/
inline constexpr float kLeastBend = 1.0e-3F;

/**
 * One lobe of a BSDF: its model, its weight at the path's wavelengths, the estimate of its albedo
 * that sampling picks it by, and what its model reads. A microfacet model reads its alphas along
 * the tangent and the bitangent, and a conductor its Fresnel factor. A dielectric reads the index
 * of refraction of the side the shading normal points away from, the outside's being one, and
 * whether it is a thin slab with the same medium on both sides.
 **/
struct Closure {
    Model model { Model::Diffuse };
    spectrum::SampledSpectrum weight { 0.0F };
    float albedo { 0.0F };
    glm::vec2 alpha { 0.5F, 0.5F };
    F82 fresnel;
    float ior { 1.5F };
    bool thin { false };

    /**
     * Whether it scatters into a single direction for each outgoing one, or two for a dielectric.
     * It does so as a mirror, as a microfacet lobe too smooth to have microfacets, or as a
     * dielectric whose index is so near one that light passes straight through. Such a closure
     * has no value and no density for any direction it is given, and only its sample finds its
     * light.
     **/
    MOYA_KERNEL bool singular() const {
        switch (model) {
            case Model::Mirror:
                return true;
            case Model::Conductor:
                return Ggx::smooth(alpha);
            case Model::Dielectric:
                return Ggx::smooth(alpha) || !(ior > 1.0F + kLeastBend);
            case Model::Diffuse:
                break;
        }
        return false;
    }

    /** The kind of bounce its sample makes: one that crosses the surface is a transmission. **/
    MOYA_KERNEL Lobe lobe(bool crossed) const {
        if (crossed) {
            return Lobe::Transmission;
        }
        if (singular()) {
            return Lobe::Specular;
        }
        return model == Model::Diffuse ? Lobe::Diffuse : Lobe::Glossy;
    }
};

};  // namespace v3d::moya::kernel::bsdf

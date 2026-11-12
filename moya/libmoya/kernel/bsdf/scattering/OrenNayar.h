/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/scattering/Diffuse.h>
#include <moya/libmoya/kernel/bsdf/scattering/Ggx.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::bsdf::scattering {

/** Fujii's constant of the Oren-Nayar lobe's normalisation: one half less two over three pi. **/
inline constexpr float kFujiiFirst = 0.5F - (2.0F / (3.0F * kPi));

/** The constant of its mean albedo: two thirds less 28 over 15 pi. **/
inline constexpr float kFujiiSecond = (2.0F / 3.0F) - (28.0F / (15.0F * kPi));

/** The least a share of lost energy is taken as, so a smooth lobe divides by no zero. **/
inline constexpr float kLeastLost = 1.0e-7F;

/**
 * OpenPBR's energy-preserving Oren-Nayar reflection, after Portsmouth, Kutz and Hill, in a frame
 * whose z is the shading normal. It reflects on the side light leaves towards, on either side.
 * The closure's colour is its reflectance, and its roughness runs from zero, Lambert's, to one.
 *
 * - **A single scattering** is Fujii's improved Oren-Nayar: the reflectance over pi, times A
 *   times one plus the roughness times s over t. A is one over one plus kFujiiFirst times the
 *   roughness. s is the dot product of the two directions less the product of their cosines, and
 *   t is the larger cosine where s is above zero, and one otherwise.
 * - **What it loses** is given back by a lobe of one minus each direction's albedo, over pi times
 *   one minus the mean albedo, times the reflectance the bounces between facets give. That
 *   reflectance is the colour squared times the mean albedo, over one less the colour times one
 *   less the mean albedo. A white lobe keeps all its energy. Both parts are reciprocal.
 * - **A sample** draws by the cosine.
 **/
struct OrenNayar {
    /** The value for two directions, before the closure's weight. **/
    MOYA_KERNEL static spectrum::SampledSpectrum evaluate(const Closure & closure, glm::vec3 outgoing, glm::vec3 incoming) {
        if (!(outgoing.z * incoming.z > 0.0F)) {
            return spectrum::SampledSpectrum(0.0F);
        }
        if (outgoing.z < 0.0F) {
            outgoing.z = -outgoing.z;
            incoming.z = -incoming.z;
        }
        const float roughness = closure.roughness;
        const float s = glm::dot(outgoing, incoming) - (outgoing.z * incoming.z);
        const float overT = s > 0.0F ? s / std::fmax(outgoing.z, incoming.z) : s;
        const float a = scale(roughness);
        const float single = a * (1.0F + (roughness * overT)) * kInversePi;
        const float mean = average(roughness);
        const float shape = std::fmax(kLeastLost, 1.0F - albedo(outgoing.z, roughness)) * std::fmax(kLeastLost, 1.0F - albedo(incoming.z, roughness)) * kInversePi /
            std::fmax(kLeastLost, 1.0F - mean);
        spectrum::SampledSpectrum out;
        for (int i = 0; i < spectrum::kSamples; ++i) {
            const float colour = closure.colour[i];
            out[i] = (colour * single) + (bounced(colour, mean) * shape);
        }
        return out;
    }

    /** What it reflects of the light along a direction at a cosine, at each wavelength. **/
    MOYA_KERNEL static spectrum::SampledSpectrum reflectance(const Closure & closure, float cosine) {
        const float once = albedo(cosine, closure.roughness);
        const float mean = average(closure.roughness);
        spectrum::SampledSpectrum out;
        for (int i = 0; i < spectrum::kSamples; ++i) {
            const float colour = closure.colour[i];
            out[i] = (colour * once) + (bounced(colour, mean) * (1.0F - once));
        }
        return out;
    }

    /** Fujii's A: the single scattering's scale at a roughness. **/
    MOYA_KERNEL static float scale(float roughness) {
        return 1.0F / (1.0F + (kFujiiFirst * roughness));
    }

    /** The single scattering's albedo, white, at a cosine and a roughness. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, then the roughness
    MOYA_KERNEL static float albedo(float cosine, float roughness) {
        const float mu = std::fmin(std::fmax(cosine, 1.0e-4F), 1.0F);
        const float sine = std::sqrt(std::fmax(0.0F, 1.0F - (mu * mu)));
        const float g = (sine * (std::acos(mu) - (sine * mu))) + ((2.0F / 3.0F) * (((sine / mu) * (1.0F - (sine * sine * sine))) - sine));
        const float a = scale(roughness);
        return a + (roughness * a * g * kInversePi);
    }

    /** The single scattering's mean albedo, white, at a roughness. **/
    MOYA_KERNEL static float average(float roughness) {
        return scale(roughness) * (1.0F + (kFujiiSecond * roughness));
    }

 private:
    /** The reflectance the bounces between facets give a colour, for a mean albedo. **/
    MOYA_KERNEL static float bounced(float colour, float mean) {
        return colour * colour * mean / (1.0F - (colour * (1.0F - mean)));
    }
};

};  // namespace v3d::moya::kernel::bsdf::scattering

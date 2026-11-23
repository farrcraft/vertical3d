/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Lobe.h>
#include <moya/libmoya/kernel/bsdf/scattering/Fibre.h>
#include <moya/libmoya/kernel/bsdf/scattering/Fresnel.h>
#include <moya/libmoya/kernel/bsdf/scattering/Ggx.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/RgbTable.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Hair.h>

#include <cmath>
#include <cstdint>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::bsdf::scattering {

/** The least and the most a hair's roughness is held to, as MaterialX holds it. **/
inline constexpr float kLeastHairRoughness = 0.001F;
inline constexpr float kMostHairRoughness = 1.0F;

/** The variance below which a lobe's longitudinal term is taken through logarithms. **/
inline constexpr float kNarrowVariance = 0.1F;

/** The terms of the series that gives the modified Bessel function of the first kind. **/
inline constexpr int kBesselTerms = 10;

/** Where the logarithm of that function is taken by its expansion for a large argument. **/
inline constexpr float kLargeBessel = 12.0F;

/** The least first random number the longitudinal term is drawn by, as pbrt-v4 holds it. **/
inline constexpr float kLeastLongitudinal = 1.0e-5F;

/**
 * Chiang and others' far-field model of a hair fibre, as pbrt-v4 gives it, with MaterialX's
 * tints and a roughness for each lobe. It works in a frame whose x lies along the fibre: a
 * direction's longitudinal sine is its x, and its azimuth its angle in y and z. Its lobes are R,
 * the reflection off the cuticle, TT, the light that passes through the fibre, TRT, the light
 * reflected once inside, and every longer path as one. Each is a longitudinal term times an
 * azimuthal one times its attenuation, from Fresnel's equations at the fibre's index and the
 * absorption along each pass through it. The last lobe's azimuthal term is uniform.
 *
 * - **Its value** is the sum of its lobes over the cosine of incoming to the frame's normal, so
 *   its projected value is the far-field model's own. A direction of no such cosine has none.
 * - **Its albedo**, with no tint and no absorption, is one, since its attenuations then sum to
 *   one and each term integrates to one.
 * - **A sample** picks a lobe by its attenuation's mean, then draws the longitudinal term and the
 *   azimuthal term exactly. Its density sums each lobe's density by its chance of being picked.
 **/
struct Hair {
    /**
     * A fibre of MaterialX's parameters at a path's wavelengths, met at an offset across its
     * width. A tint is a reflectance, and the absorption an unbounded spectrum. Each roughness is
     * held to [kLeastHairRoughness, kMostHairRoughness]. The cuticle angle a turns the outgoing
     * direction of R by twice pi (a - 1/2), TT's by pi (1/2 - a) and TRT's by four times that.
     **/
    MOYA_KERNEL static Fibre fibre(const surface::Hair & hair, const spectrum::RgbTable & table, const spectrum::SampledWavelengths & wavelengths, float offset) {
        Fibre out;
        out.tints[0] = spectrum::reflectance(table, hair.tintR, wavelengths);
        out.tints[1] = spectrum::reflectance(table, hair.tintTT, wavelengths);
        out.tints[2] = spectrum::reflectance(table, hair.tintTRT, wavelengths);
        out.absorption = spectrum::unbounded(table, hair.absorption, wavelengths);
        out.ior = hair.ior;
        const glm::vec2 roughness[kHairShaped] { hair.roughnessR, hair.roughnessTT, hair.roughnessTRT };
        for (std::uint32_t order = 0; order < kHairShaped; ++order) {
            out.variance[order] = held(roughness[order].x);
            out.scale[order] = kLogisticScale * held(roughness[order].y);
        }
        out.variance[kHairShaped] = out.variance[kHairShaped - 1U];
        const float tilt = kPi * (0.5F - hair.cuticleAngle);
        const float turns[kHairShaped] { -2.0F * tilt, tilt, 4.0F * tilt };
        for (std::uint32_t order = 0; order < kHairShaped; ++order) {
            out.tiltSine[order] = std::sin(turns[order]);
            out.tiltCosine[order] = std::cos(turns[order]);
        }
        out.offset = std::fmin(std::fmax(offset, -1.0F), 1.0F);
        return out;
    }

    /** The value for two directions in the fibre's frame. **/
    MOYA_KERNEL static spectrum::SampledSpectrum evaluate(const Fibre & fibre, const glm::vec3 & outgoing, const glm::vec3 & incoming) {
        const float cosine = std::fabs(incoming.z);
        if (!(cosine > 0.0F)) {
            return spectrum::SampledSpectrum(0.0F);
        }
        const Lobes lobes = lobesOf(fibre, outgoing, incoming);
        spectrum::SampledSpectrum out(0.0F);
        for (const spectrum::SampledSpectrum & lobe : lobes.values) {
            out += lobe;
        }
        return out / cosine;
    }

    /** The density sample() draws incoming with, for outgoing. **/
    MOYA_KERNEL static float density(const Fibre & fibre, const glm::vec3 & outgoing, const glm::vec3 & incoming) {
        return lobesOf(fibre, outgoing, incoming).density;
    }

    /**
     * A direction light comes in from for outgoing, from two random numbers and a third, which
     * picks the lobe and is then stretched over the lobe's share to draw its azimuth. The kind of
     * bounce is TT's transmission, or a glossy reflection for every other lobe. A direction of no
     * length is no sample.
     **/
    MOYA_KERNEL static glm::vec3 sample(const Fibre & fibre, const glm::vec3 & outgoing, const glm::vec2 & u, float choice, Lobe & lobe) {
        const Outgoing seen = outgoingOf(fibre, outgoing);
        if (!seen.found) {
            return glm::vec3 { 0.0F };
        }
        // the last lobe of any chance takes a number its rounded chances leave over
        std::uint32_t picked = kHairLobes;
        float below = 0.0F;
        float before = 0.0F;
        for (std::uint32_t order = 0; order < kHairLobes; ++order) {
            if (seen.chances[order] > 0.0F) {
                picked = order;
                before = below;
                if (choice < below + seen.chances[order]) {
                    break;
                }
            }
            below += seen.chances[order];
        }
        if (picked >= kHairLobes) {
            return glm::vec3 { 0.0F };
        }
        const float stretched = std::fmin(std::fmax((choice - before) / seen.chances[picked], 0.0F), 0x1.fffffep-1F);
        const Tilted turned = tilted(fibre, picked, seen);
        const float variance = fibre.variance[picked];
        const float cosine = 1.0F + (variance * std::log(std::fmax(u.x, kLeastLongitudinal) + ((1.0F - u.x) * std::exp(-2.0F / variance))));
        const float sine = rootOf(1.0F - (cosine * cosine));
        const float sinIncoming = (-cosine * turned.sine) + (sine * std::cos(2.0F * kPi * u.y) * turned.cosine);
        const float cosIncoming = rootOf(1.0F - (sinIncoming * sinIncoming));
        float turn = 2.0F * kPi * stretched;
        if (picked < kHairShaped) {
            turn = azimuthOf(picked, seen.gammaO, seen.gammaT) + trimmedSample(stretched, fibre.scale[picked]);
        }
        const float azimuth = seen.azimuth + turn;
        lobe = picked == 1U ? Lobe::Transmission : Lobe::Glossy;
        return glm::vec3 { sinIncoming, cosIncoming * std::cos(azimuth), cosIncoming * std::sin(azimuth) };
    }

    /**
     * The kind of bounce that carries most of the light between two directions: TT's
     * transmission, or a glossy reflection.
     **/
    MOYA_KERNEL static Lobe dominant(const Fibre & fibre, const glm::vec3 & outgoing, const glm::vec3 & incoming) {
        const Lobes lobes = lobesOf(fibre, outgoing, incoming);
        float most = 0.0F;
        std::uint32_t chosen = 0;
        for (std::uint32_t order = 0; order < kHairLobes; ++order) {
            if (lobes.values[order].average() > most) {
                most = lobes.values[order].average();
                chosen = order;
            }
        }
        return chosen == 1U ? Lobe::Transmission : Lobe::Glossy;
    }

    /**
     * What the fibre scatters of light that leaves it at right angles to its length: the sum of
     * its lobes' tinted attenuations there, at each wavelength.
     **/
    MOYA_KERNEL static spectrum::SampledSpectrum albedo(const Fibre & fibre) {
        const Outgoing seen = outgoingOf(fibre, glm::vec3 { 0.0F, 0.0F, 1.0F });
        spectrum::SampledSpectrum out(0.0F);
        for (const spectrum::SampledSpectrum & attenuation : seen.attenuations) {
            out += attenuation;
        }
        return out;
    }

    /** A roughness held to [kLeastHairRoughness, kMostHairRoughness]. **/
    MOYA_KERNEL static float held(float roughness) {
        return std::fmin(std::fmax(roughness, kLeastHairRoughness), kMostHairRoughness);
    }

    /** The modified Bessel function of the first kind and of order zero, by its series. **/
    MOYA_KERNEL static float besselI0(float x) {
        float out = 0.0F;
        float term = 1.0F;
        const float quarter = 0.25F * x * x;
        for (int i = 0; i < kBesselTerms; ++i) {
            if (i > 0) {
                term *= quarter / static_cast<float>(i * i);
            }
            out += term;
        }
        return out;
    }

    /** The logarithm of besselI0(), by its expansion where its argument is large. **/
    MOYA_KERNEL static float logBesselI0(float x) {
        if (x > kLargeBessel) {
            // NOLINTNEXTLINE(readability-magic-numbers) - the expansion's own terms
            return x + (0.5F * (-std::log(2.0F * kPi) + std::log(1.0F / x) + (1.0F / (8.0F * x))));
        }
        return std::log(besselI0(x));
    }

    /**
     * A lobe's longitudinal term for incoming's and the turned outgoing's sines and cosines, of a
     * variance. It integrates to one against the cosine of incoming's longitudinal angle.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - cosines, then sines, as pbrt-v4's
    MOYA_KERNEL static float longitudinal(float cosIncoming, float cosOutgoing, float sinIncoming, float sinOutgoing, float variance) {
        const float cosines = cosIncoming * cosOutgoing / variance;
        const float sines = sinIncoming * sinOutgoing / variance;
        if (variance <= kNarrowVariance) {
            return std::exp(logBesselI0(cosines) - sines - (1.0F / variance) + 0.6931F + std::log(1.0F / (2.0F * variance)));  // NOLINT(readability-magic-numbers) - log 2
        }
        return std::exp(-sines) * besselI0(cosines) / (std::sinh(1.0F / variance) * 2.0F * variance);
    }

    /** A logistic of a scale held to [-pi, pi], at an angle in that range. **/
    MOYA_KERNEL static float trimmed(float angle, float scale) {
        const float away = std::exp(-std::fabs(angle) / scale);
        const float logistic = away / (scale * (1.0F + away) * (1.0F + away));
        return logistic / (cumulative(kPi, scale) - cumulative(-kPi, scale));
    }

 private:
    /** What the lobes share for an outgoing direction. **/
    struct Outgoing {
        bool found { false };
        float sine { 0.0F };
        float cosine { 0.0F };
        float azimuth { 0.0F };
        float gammaO { 0.0F };
        float gammaT { 0.0F };
        spectrum::SampledSpectrum attenuations[kHairLobes];
        float chances[kHairLobes] { 0.0F, 0.0F, 0.0F, 0.0F };
    };

    /** Each lobe's value for two directions, before the cosine, and their density together. **/
    struct Lobes {
        spectrum::SampledSpectrum values[kHairLobes];
        float density { 0.0F };
    };

    /** A lobe's outgoing sine and cosine, turned by the cuticle's tilt. **/
    struct Tilted {
        float sine { 0.0F };
        float cosine { 1.0F };
    };

    /** The root of a value, or zero for one below zero. **/
    MOYA_KERNEL static float rootOf(float value) {
        return std::sqrt(std::fmax(value, 0.0F));
    }

    /** The arcsine of a value held to [-1, 1]. **/
    MOYA_KERNEL static float arcsine(float value) {
        return std::asin(std::fmin(std::fmax(value, -1.0F), 1.0F));
    }

    /** A logistic's cumulative distribution, of a scale. **/
    MOYA_KERNEL static float cumulative(float angle, float scale) {
        return 1.0F / (1.0F + std::exp(-angle / scale));
    }

    /** An angle drawn from the trimmed logistic of a scale, from a random number. **/
    MOYA_KERNEL static float trimmedSample(float u, float scale) {
        const float low = cumulative(-kPi, scale);
        const float within = low + (u * (cumulative(kPi, scale) - low));
        const float out = -scale * std::log((1.0F / within) - 1.0F);
        return std::fmin(std::fmax(out, -kPi), kPi);
    }

    /** The azimuth a lobe of a path through the fibre leaves at, relative to outgoing's. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - outgoing's angle, then the refracted
    MOYA_KERNEL static float azimuthOf(std::uint32_t lobe, float gammaO, float gammaT) {
        const auto order = static_cast<float>(lobe);
        return (2.0F * order * gammaT) - (2.0F * gammaO) + (order * kPi);
    }

    /** A lobe's azimuthal term for a difference of azimuths, taken within [-pi, pi]. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - outgoing's angle, then the refracted
    MOYA_KERNEL static float azimuthal(float difference, std::uint32_t lobe, float scale, float gammaO, float gammaT) {
        float turned = difference - azimuthOf(lobe, gammaO, gammaT);
        if (!std::isfinite(turned)) {
            return 0.0F;
        }
        turned -= 2.0F * kPi * std::round(turned / (2.0F * kPi));
        return trimmed(std::fmin(std::fmax(turned, -kPi), kPi), scale);
    }

    /** A lobe's outgoing sine and cosine, turned by the tilt, the last lobe's not at all. **/
    MOYA_KERNEL static Tilted tilted(const Fibre & fibre, std::uint32_t lobe, const Outgoing & seen) {
        if (lobe >= kHairShaped) {
            return Tilted { .sine = seen.sine, .cosine = seen.cosine };
        }
        const float sine = (seen.sine * fibre.tiltCosine[lobe]) + (seen.cosine * fibre.tiltSine[lobe]);
        const float cosine = (seen.cosine * fibre.tiltCosine[lobe]) - (seen.sine * fibre.tiltSine[lobe]);
        return Tilted { .sine = sine, .cosine = std::fabs(cosine) };
    }

    /**
     * What the lobes share for an outgoing direction: its angles, the angles of the path inside
     * the fibre, each lobe's tinted attenuation and its chance of being picked. A direction along
     * the fibre is not found.
     **/
    MOYA_KERNEL static Outgoing outgoingOf(const Fibre & fibre, const glm::vec3 & outgoing) {
        Outgoing out;
        out.sine = std::fmin(std::fmax(outgoing.x, -1.0F), 1.0F);
        out.cosine = rootOf(1.0F - (out.sine * out.sine));
        if (!(out.cosine > 0.0F) || !(fibre.ior > 0.0F)) {
            return out;
        }
        out.azimuth = std::atan2(outgoing.z, outgoing.y);
        const float offset = fibre.offset;
        out.gammaO = arcsine(offset);
        const float sinT = out.sine / fibre.ior;
        const float cosT = rootOf(1.0F - (sinT * sinT));
        const float etap = rootOf((fibre.ior * fibre.ior) - (out.sine * out.sine)) / out.cosine;
        const float sinGammaT = etap > 0.0F ? offset / etap : 0.0F;
        const float cosGammaT = rootOf(1.0F - (sinGammaT * sinGammaT));
        out.gammaT = arcsine(sinGammaT);
        spectrum::SampledSpectrum through(0.0F);
        for (int w = 0; w < spectrum::kSamples; ++w) {
            through[w] = cosT > 0.0F ? std::exp(-fibre.absorption[w] * (2.0F * cosGammaT / cosT)) : 0.0F;
        }
        const float reflected = Fresnel::reflected(out.cosine * rootOf(1.0F - (offset * offset)), fibre.ior);
        out.attenuations[0] = spectrum::SampledSpectrum(reflected);
        out.attenuations[1] = through * ((1.0F - reflected) * (1.0F - reflected));
        out.attenuations[2] = out.attenuations[1] * through * reflected;
        for (int w = 0; w < spectrum::kSamples; ++w) {
            const float kept = through[w] * reflected;
            out.attenuations[3][w] = kept < 1.0F ? out.attenuations[2][w] * kept / (1.0F - kept) : 0.0F;
        }
        // the last lobe takes TRT's tint
        out.attenuations[kHairShaped] *= fibre.tints[kHairShaped - 1U];
        float sum = out.attenuations[kHairShaped].average();
        for (std::uint32_t order = 0; order < kHairShaped; ++order) {
            out.attenuations[order] *= fibre.tints[order];
            sum += out.attenuations[order].average();
        }
        if (!(sum > 0.0F)) {
            return out;
        }
        for (std::uint32_t order = 0; order < kHairLobes; ++order) {
            out.chances[order] = out.attenuations[order].average() / sum;
        }
        out.found = true;
        return out;
    }

    /** Each lobe's value and the density for two directions, as Lobes holds them. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - outgoing, then incoming
    MOYA_KERNEL static Lobes lobesOf(const Fibre & fibre, const glm::vec3 & outgoing, const glm::vec3 & incoming) {
        Lobes out;
        const Outgoing seen = outgoingOf(fibre, outgoing);
        if (!seen.found) {
            return out;
        }
        const float sinIncoming = std::fmin(std::fmax(incoming.x, -1.0F), 1.0F);
        const float cosIncoming = rootOf(1.0F - (sinIncoming * sinIncoming));
        const float difference = std::atan2(incoming.z, incoming.y) - seen.azimuth;
        for (std::uint32_t order = 0; order < kHairLobes; ++order) {
            const Tilted turned = tilted(fibre, order, seen);
            const float along = longitudinal(cosIncoming, turned.cosine, sinIncoming, turned.sine, fibre.variance[order]);
            const float around = order < kHairShaped ? azimuthal(difference, order, fibre.scale[order], seen.gammaO, seen.gammaT) : 1.0F / (2.0F * kPi);
            out.values[order] = seen.attenuations[order] * (along * around);
            out.density += seen.chances[order] * along * around;
        }
        return out;
    }
};

};  // namespace v3d::moya::kernel::bsdf::scattering

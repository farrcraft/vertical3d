/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/RgbTable.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/volume/Medium.h>
#include <moya/libmoya/kernel/volume/Phase.h>
#include <moya/libmoya/kernel/volume/PhaseSample.h>

#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::volume {

/**
 * A medium's coefficients at the wavelengths a path carries: its absorption, and each lobe's
 * scattering and anisotropy. Each coefficient is the unbounded spectrum of its ACEScg colour.
 **/
struct Coefficients {
    spectrum::SampledSpectrum absorption;
    spectrum::SampledSpectrum scattering[kMostLobes];
    float anisotropy[kMostLobes] {};

    /** What every lobe scatters together. **/
    MOYA_KERNEL spectrum::SampledSpectrum scattered() const {
        spectrum::SampledSpectrum out(0.0F);
        for (const spectrum::SampledSpectrum & lobe : scattering) {
            out += lobe;
        }
        return out;
    }

    /** What the medium absorbs and scatters together. **/
    MOYA_KERNEL spectrum::SampledSpectrum extinction() const {
        return absorption + scattered();
    }
};

/** A medium's coefficients at the wavelengths a path carries. **/
MOYA_KERNEL inline Coefficients coefficientsOf(const spectrum::RgbTable & table, const Medium & medium, const spectrum::SampledWavelengths & wavelengths) {
    Coefficients out;
    out.absorption = spectrum::unbounded(table, medium.absorption, wavelengths);
    for (std::uint32_t lobe = 0; lobe < kMostLobes; ++lobe) {
        out.scattering[lobe] = spectrum::unbounded(table, medium.lobes[lobe].scattering, wavelengths);
        out.anisotropy[lobe] = medium.lobes[lobe].anisotropy;
    }
    return out;
}

/**
 * The share of a medium's scattering each lobe holds, over the wavelengths a path carries
 * together. A direction is drawn by these shares, so its density is the same whichever wavelength
 * a path tracks by.
 **/
MOYA_KERNEL inline void lobeShares(const Coefficients & coefficients, float (&shares)[kMostLobes]) {
    float sum = 0.0F;
    for (std::uint32_t lobe = 0; lobe < kMostLobes; ++lobe) {
        shares[lobe] = coefficients.scattering[lobe].average();
        sum += shares[lobe];
    }
    for (float & share : shares) {
        share = sum > 0.0F ? share / sum : 0.0F;
    }
}

/**
 * The phase function of a medium at each wavelength, at the cosine between the direction a path
 * arrives along and the one it leaves along. It is each lobe's Henyey and Greenstein's function,
 * weighed by its share of the scattering at that wavelength. A wavelength that nothing scatters
 * at takes zero.
 **/
MOYA_KERNEL inline spectrum::SampledSpectrum phaseValue(const Coefficients & coefficients, float cosine) {
    const spectrum::SampledSpectrum scattered = coefficients.scattered();
    spectrum::SampledSpectrum out(0.0F);
    for (std::uint32_t lobe = 0; lobe < kMostLobes; ++lobe) {
        out += coefficients.scattering[lobe] * henyeyGreenstein(cosine, coefficients.anisotropy[lobe]);
    }
    return spectrum::safeDivide(out, scattered);
}

/** The density with which phaseSample() draws a direction at a cosine to the one arrived along. **/
MOYA_KERNEL inline float phaseDensity(const Coefficients & coefficients, float cosine) {
    float shares[kMostLobes];
    lobeShares(coefficients, shares);
    float out = 0.0F;
    for (std::uint32_t lobe = 0; lobe < kMostLobes; ++lobe) {
        out += shares[lobe] > 0.0F ? shares[lobe] * henyeyGreenstein(cosine, coefficients.anisotropy[lobe]) : 0.0F;
    }
    return out;
}

/**
 * A direction drawn from a medium's phase function, about the direction a path arrives along. A
 * lobe is drawn by its share from choice, and the direction from that lobe by u. The density is
 * phaseDensity()'s, over every lobe.
 **/
MOYA_KERNEL inline PhaseSample phaseSample(const Coefficients & coefficients, const glm::vec3 & forward, const glm::vec2 & u, float choice) {
    float shares[kMostLobes];
    lobeShares(coefficients, shares);
    std::uint32_t chosen = kMostLobes;
    float below = 0.0F;
    for (std::uint32_t lobe = 0; lobe < kMostLobes; ++lobe) {
        if (shares[lobe] > 0.0F) {
            chosen = lobe;
            below += shares[lobe];
            if (choice < below) {
                break;
            }
        }
    }
    PhaseSample out;
    if (chosen == kMostLobes) {
        return out;
    }
    out.incoming = henyeyGreensteinSample(forward, coefficients.anisotropy[chosen], u);
    const float cosine = glm::dot(forward, out.incoming);
    out.value = phaseValue(coefficients, cosine);
    out.density = phaseDensity(coefficients, cosine);
    return out;
}

};  // namespace v3d::moya::kernel::volume

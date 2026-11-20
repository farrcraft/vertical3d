/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>

#include <cmath>

namespace v3d::moya::kernel::bsdf::scattering {

/** Pi, for the film's phase. **/
inline constexpr float kFilmPi = 3.14159265F;

/**
 * The wavelength in nanometres an RGB build evaluates a film at for each of ACEScg's channels:
 * one near the middle of what each primary carries.
 **/
inline constexpr float kRgbWavelengths[3] = { 630.0F, 532.0F, 465.0F };

/**
 * A thin film over a surface, in nanometres thick and of an index relative to the medium over
 * it. Light reflected from its top and from the surface below interferes, so its reflectance
 * depends on the wavelength, which a path carries four of.
 *
 * The reflectance is Airy's sum over the light bouncing inside the film, for each polarisation,
 * from the amplitudes each interface reflects, then averaged. The phase between successive
 * bounces is four pi times the film's index, its thickness and the cosine inside it, over the
 * wavelength. Light that cannot enter the film is all reflected. A film of no thickness is no
 * film.
 **/
struct ThinFilm {
    float thickness { 0.0F };
    float ior { 1.4F };

    /** Whether it is there at all. **/
    MOYA_KERNEL bool present() const {
        return thickness > 0.0F;
    }

    /**
     * Its reflectance over a dielectric of an index relative to the medium over the film, at the
     * cosine light arrives at, for each wavelength.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, then the base's index
    MOYA_KERNEL spectrum::SampledSpectrum overDielectric(float cosine, float base, const spectrum::SampledWavelengths & wavelengths) const {
        spectrum::SampledSpectrum out(1.0F);
        const float inside = refracted(cosine, ior);
        if (!(inside > 0.0F)) {
            return out;
        }
        const float below = refracted(cosine, base);
        const Amplitudes top = amplitudes(1.0F, ior, cosine, inside);
        const Amplitudes bottom = below > 0.0F ? amplitudes(ior, base, inside, below) : Amplitudes { 1.0F, 1.0F };
        for (int i = 0; i < spectrum::kSamples; ++i) {
            const float phase = phaseAt(inside, wavelengthAt(wavelengths, i));
            out[i] = 0.5F * (airy(top.perpendicular, bottom.perpendicular, phase) + airy(top.parallel, bottom.parallel, phase));
        }
        return out;
    }

    /**
     * Its reflectance over a conductor, at the cosine light arrives at, for each wavelength. The
     * conductor reflects what reflected gives at the cosine inside the film, for both
     * polarisations. Its amplitude turns back the phase where the index its reflectance at normal
     * incidence implies lies below the film's.
     **/
    template <typename Reflected>
    MOYA_KERNEL spectrum::SampledSpectrum overConductor(float cosine, const spectrum::SampledSpectrum & normal, Reflected reflected,
        const spectrum::SampledWavelengths & wavelengths) const {
        spectrum::SampledSpectrum out(1.0F);
        const float inside = refracted(cosine, ior);
        if (!(inside > 0.0F)) {
            return out;
        }
        const Amplitudes top = amplitudes(1.0F, ior, cosine, inside);
        const spectrum::SampledSpectrum base = reflected(inside);
        for (int i = 0; i < spectrum::kSamples; ++i) {
            const float root = std::sqrt(std::fmin(std::fmax(normal[i], 0.0F), 0.9999F));
            const float index = (1.0F + root) / (1.0F - root);
            const float amplitude = std::sqrt(std::fmax(base[i], 0.0F)) * (index < ior ? -1.0F : 1.0F);
            const float phase = phaseAt(inside, wavelengthAt(wavelengths, i));
            out[i] = 0.5F * (airy(top.perpendicular, amplitude, phase) + airy(top.parallel, amplitude, phase));
        }
        return out;
    }

    /**
     * Airy's reflectance of a film whose top reflects the amplitude first and whose bottom
     * reflects second, at a phase between bounces.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the two amplitudes, then the phase
    MOYA_KERNEL static float airy(float first, float second, float phase) {
        const float cross = 2.0F * first * second * std::cos(phase);
        return ((first * first) + (second * second) + cross) / (1.0F + (first * first * second * second) + cross);
    }

 private:
    /** The amplitudes an interface reflects, perpendicular and parallel to the plane of light. **/
    struct Amplitudes {
        float perpendicular { 0.0F };
        float parallel { 0.0F };
    };

    /**
     * The cosine light refracts to in a medium of an index, from one of index one. It is zero
     * beyond the critical angle.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, then the index
    MOYA_KERNEL static float refracted(float cosine, float index) {
        const float sine = (1.0F - (cosine * cosine)) / (index * index);
        return sine < 1.0F ? std::sqrt(1.0F - sine) : 0.0F;
    }

    /** The amplitudes reflected from a medium of one index into another, at their cosines. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the two indices, then the two cosines
    MOYA_KERNEL static Amplitudes amplitudes(float from, float to, float arriving, float leaving) {
        return Amplitudes { .perpendicular = ((from * arriving) - (to * leaving)) / ((from * arriving) + (to * leaving)),
            .parallel = ((to * arriving) - (from * leaving)) / ((to * arriving) + (from * leaving)) };
    }

    /** The phase between successive bounces inside the film at a cosine, for a wavelength. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, then the wavelength
    MOYA_KERNEL float phaseAt(float cosine, float wavelength) const {
        // light crosses the film twice, and each crossing adds 2 pi n d cos / wavelength
        return 2.0F * (2.0F * kFilmPi * ior * thickness * cosine / wavelength);
    }

    /** A path's wavelength, or an RGB build's for its channel. **/
    MOYA_KERNEL static float wavelengthAt(const spectrum::SampledWavelengths & wavelengths, int index) {
#ifdef MOYA_RGB
        static_cast<void>(wavelengths);
        return kRgbWavelengths[index];
#else
        return wavelengths.lambda[index];
#endif
    }
};

};  // namespace v3d::moya::kernel::bsdf::scattering

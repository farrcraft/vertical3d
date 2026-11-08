/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

/**
 * Light carried as values at a few wavelengths: the sampled spectrum, the wavelengths a path
 * carries, and their conversion to colour through the CIE's observer.
 **/
namespace v3d::moya::kernel::spectrum {

#ifdef MOYA_RGB
/** How many values a colour holds: an RGB build carries ACEScg's three channels. **/
inline constexpr int kSamples = 3;
#else
/** How many wavelengths each path carries. **/
inline constexpr int kSamples = 4;
#endif

/**
 * A spectral quantity at the wavelengths of one path: radiance, reflectance or a throughput, one
 * value for each of the path's wavelengths, in the same order as its SampledWavelengths. In an RGB
 * build, which defines MOYA_RGB, it holds ACEScg's red, green and blue instead.
 *
 * Arithmetic is per wavelength, or per channel.
 **/
struct SampledSpectrum {
    float values[kSamples] {};

    /** Every value zero. **/
    MOYA_KERNEL SampledSpectrum() = default;

    /** Every value the same. **/
    MOYA_KERNEL explicit SampledSpectrum(float value) {
        for (float & each : values) {
            each = value;
        }
    }

    MOYA_KERNEL float & operator[](int index) {
        return values[index];
    }

    MOYA_KERNEL float operator[](int index) const {
        return values[index];
    }

    /** The mean of the values. **/
    MOYA_KERNEL float average() const {
        float sum = 0.0F;
        for (const float each : values) {
            sum += each;
        }
        return sum / static_cast<float>(kSamples);
    }

    /** The largest value. **/
    MOYA_KERNEL float maximum() const {
        float most = values[0];
        for (const float each : values) {
            most = each > most ? each : most;
        }
        return most;
    }

    /** Whether every value is zero. **/
    MOYA_KERNEL bool black() const {
        bool zero = true;
        for (const float each : values) {
            zero = zero && each == 0.0F;
        }
        return zero;
    }

    MOYA_KERNEL SampledSpectrum & operator+=(const SampledSpectrum & other) {
        for (int i = 0; i < kSamples; ++i) {
            values[i] += other.values[i];
        }
        return *this;
    }

    MOYA_KERNEL SampledSpectrum & operator-=(const SampledSpectrum & other) {
        for (int i = 0; i < kSamples; ++i) {
            values[i] -= other.values[i];
        }
        return *this;
    }

    MOYA_KERNEL SampledSpectrum & operator*=(const SampledSpectrum & other) {
        for (int i = 0; i < kSamples; ++i) {
            values[i] *= other.values[i];
        }
        return *this;
    }

    MOYA_KERNEL SampledSpectrum & operator*=(float scale) {
        for (float & each : values) {
            each *= scale;
        }
        return *this;
    }

    /** Divides by a scale, which is not zero. **/
    MOYA_KERNEL SampledSpectrum & operator/=(float scale) {
        for (float & each : values) {
            each /= scale;
        }
        return *this;
    }
};

MOYA_KERNEL inline SampledSpectrum operator+(SampledSpectrum left, const SampledSpectrum & right) {
    return left += right;
}

MOYA_KERNEL inline SampledSpectrum operator-(SampledSpectrum left, const SampledSpectrum & right) {
    return left -= right;
}

MOYA_KERNEL inline SampledSpectrum operator*(SampledSpectrum left, const SampledSpectrum & right) {
    return left *= right;
}

MOYA_KERNEL inline SampledSpectrum operator*(SampledSpectrum spectrum, float scale) {
    return spectrum *= scale;
}

MOYA_KERNEL inline SampledSpectrum operator*(float scale, SampledSpectrum spectrum) {
    return spectrum *= scale;
}

MOYA_KERNEL inline SampledSpectrum operator/(SampledSpectrum spectrum, float scale) {
    return spectrum /= scale;
}

/** The numerator over the denominator at each wavelength, and zero where the denominator is. **/
MOYA_KERNEL inline SampledSpectrum safeDivide(const SampledSpectrum & numerator, const SampledSpectrum & denominator) {
    SampledSpectrum out;
    for (int i = 0; i < kSamples; ++i) {
        out.values[i] = denominator.values[i] != 0.0F ? numerator.values[i] / denominator.values[i] : 0.0F;
    }
    return out;
}

};  // namespace v3d::moya::kernel::spectrum

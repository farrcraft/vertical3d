/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/Launch.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/kernel/spectrum/Colorimetry.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::film {

/** The components of the beauty, ACEScg's red, green and blue, which start each pixel. **/
inline constexpr std::uint32_t kBeautyComponents = 3;

/**
 * The floats between the channels and the weight that measure a pixel's noise. Each is a sum over
 * the pixel's samples of the squared weight: times the squared luminance, times the luminance,
 * and alone.
 **/
inline constexpr std::uint32_t kMomentFloats = 3;

/**
 * A film's pixels in device memory, which kernel code adds samples to. Each pixel holds, in
 * order, the beauty's weighted sums, each channel's weighted sums, the beauty's moments, and the
 * sum of its samples' weights. The host divides the sums by the weight when it reads them, and
 * gives the standard error of the beauty's luminance from the moments.
 *
 * Each pixel belongs to one work item, which adds the pixel's samples in index order, so its sums
 * are the same under any thread count.
 **/
struct Pixels {
    Array<float> values;

    /** The image's width, in pixels. **/
    std::uint32_t width { 0 };

    /** The floats each pixel holds, the weight included. **/
    std::uint32_t stride { 0 };

    /** A pixel's first float. **/
    MOYA_KERNEL float * at(Item pixel) const {
        return &values[((pixel.y * width) + pixel.x) * stride];
    }

    /**
     * Adds a camera sample's radiance, with its filter weight, to the beauty, its luminance to the
     * beauty's moments, and the weight to the pixel's sum of weights. The radiance is converted to
     * ACEScg through the observer.
     **/
    MOYA_KERNEL void add(Item pixel, float weight, const spectrum::SampledSpectrum & radiance,
        const spectrum::SampledWavelengths & wavelengths) const {
        const glm::vec3 xyz = spectrum::toXyz(radiance, wavelengths);
        const glm::vec3 rgb = spectrum::acescgFromXyz(xyz);
        float * const out = at(pixel);
        for (std::uint32_t i = 0; i < kBeautyComponents; ++i) {
            out[i] += weight * rgb[static_cast<glm::length_t>(i)];
        }
        const float squared = weight * weight;
        float * const moments = out + (stride - 1U - kMomentFloats);
        moments[0] += squared * xyz.y * xyz.y;
        moments[1] += squared * xyz.y;
        moments[2] += squared;
        out[stride - 1U] += weight;
    }

    /**
     * Adds a value, with the same sample's filter weight, to a channel whose first component is
     * at the given offset. The weight is not added again: add() adds it once per sample.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the weight first, as in add()
    MOYA_KERNEL void addChannel(Item pixel, std::uint32_t offset, float weight, float value) const {
        at(pixel)[offset] += weight * value;
    }

    /** Adds a value of several components to a channel, as the one-component addChannel() does. **/
    template <glm::length_t N>
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the weight first, as in add()
    MOYA_KERNEL void addChannel(Item pixel, std::uint32_t offset, float weight, const glm::vec<N, float> & value) const {
        float * const out = at(pixel) + offset;
        for (glm::length_t i = 0; i < N; ++i) {
            out[i] += weight * value[i];
        }
    }
};

};  // namespace v3d::moya::kernel::film

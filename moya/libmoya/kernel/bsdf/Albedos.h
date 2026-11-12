/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>
#include <cstdint>

namespace v3d::moya::kernel::bsdf {

/**
 * The albedo of a GGX reflection with a Fresnel factor of one, for energy compensation: how much
 * of the light from a direction it reflects in one scattering, and its mean over the hemisphere.
 *
 * The directional albedo is held at a grid of cosines and roughnesses, each from zero to one, the
 * roughness being the square root of the alpha. It is read between them bilinearly. The mean is
 * twice the integral of the cosine times that reading over the cosine, so a compensation lobe
 * built from both returns what a single scattering loses, within the table's error.
 *
 * A table of no points reads an albedo of one everywhere, which compensates nothing.
 **/
struct Albedos {
    /** The points along the cosine and along the roughness. **/
    std::uint32_t cosines { 0 };
    std::uint32_t roughnesses { 0 };

    /** The directional albedo, by roughness, then by cosine. **/
    Array<const float> directional;

    /** The mean albedo at each roughness. **/
    Array<const float> average;

    /** The directional albedo at a cosine and an alpha. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, then the alpha
    MOYA_KERNEL float at(float cosine, float alpha) const {
        if (cosines < 2U || roughnesses < 2U) {
            return 1.0F;
        }
        const Place across = place(cosine, cosines);
        const Place up = place(std::sqrt(alpha), roughnesses);
        const auto read = [&](std::uint32_t row) {
            const float low = directional[(row * cosines) + across.index];
            const float high = directional[(row * cosines) + across.index + 1U];
            return low + ((high - low) * across.fraction);
        };
        const float low = read(up.index);
        return low + ((read(up.index + 1U) - low) * up.fraction);
    }

    /** The mean albedo at an alpha. **/
    MOYA_KERNEL float averageAt(float alpha) const {
        if (cosines < 2U || roughnesses < 2U) {
            return 1.0F;
        }
        const Place up = place(std::sqrt(alpha), roughnesses);
        const float low = average[up.index];
        return low + ((average[up.index + 1U] - low) * up.fraction);
    }

 private:
    /** Where a value from zero to one falls among points: the point below, and how far on. **/
    struct Place {
        std::uint32_t index { 0 };
        float fraction { 0.0F };
    };

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the value, then the count of points
    MOYA_KERNEL static Place place(float value, std::uint32_t count) {
        const auto last = static_cast<float>(count - 1U);
        const float scaled = std::fmin(std::fmax(value, 0.0F), 1.0F) * last;
        const auto index = static_cast<std::uint32_t>(std::fmin(std::floor(scaled), last - 1.0F));  // checked: scaled is held to [0, last]
        return Place { .index = index, .fraction = scaled - static_cast<float>(index) };
    }
};

};  // namespace v3d::moya::kernel::bsdf

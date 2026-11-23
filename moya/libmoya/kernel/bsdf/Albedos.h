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

/** The largest of the ior axis's values, (eta - 1) / (eta + 1): an index of four. **/
inline constexpr float kMostIorAxis = 0.6F;

/**
 * The albedos of GGX's single scattering, which energy compensation reads: how much of the light
 * from a direction one scattering sends on, and its mean over the hemisphere. Each is held at a
 * grid of points and read between them linearly along each axis.
 *
 * - **The cosine** of the direction to the shading normal runs from zero to one.
 * - **The roughness** is the square root of the alpha, from zero to one.
 * - **The ior axis** is (eta - 1) / (eta + 1) for a ratio of indices eta of at least one, from
 *   zero to kMostIorAxis. A larger eta is read at the axis's end.
 *
 * A reflection's table holds a GGX reflection with a Fresnel factor of one. A dielectric's tables
 * hold its reflection and transmission together, by Fresnel's equations, on each side: entering
 * from outside at eta, and leaving from inside at one over eta. Its transmission is counted by
 * the energy it carries, not by its radiance. A mean is twice the integral of the cosine times
 * the directional albedo.
 *
 * Two values at each point of the ior axis complete them: the share of light a smooth interface
 * transmits from outside, and the share a thin slab reflects, each a mean over the hemisphere. A
 * last table holds a dielectric's reflection alone from outside, as a layer over a base reflects.
 *
 * A table of no points reads an albedo of one everywhere, which compensates nothing, and shares
 * and a reflection alone of zero.
 **/
struct Albedos {
    /** The points along the cosine and along the roughness. **/
    std::uint32_t cosines { 0 };
    std::uint32_t roughnesses { 0 };

    /** The reflection's directional albedo, by roughness, then by cosine. **/
    Array<const float> directional;

    /** The reflection's mean albedo at each roughness. **/
    Array<const float> average;

    /** The points along the ior axis. **/
    std::uint32_t iors { 0 };

    /** A dielectric's directional albedo from outside and inside, by ior, roughness and cosine. **/
    Array<const float> entering;
    Array<const float> leaving;

    /** A dielectric's mean albedo from outside and from inside, by ior, then by roughness. **/
    Array<const float> enteringAverage;
    Array<const float> leavingAverage;

    /** The mean share a smooth interface transmits from outside, and a thin slab reflects. **/
    Array<const float> transmitted;
    Array<const float> slab;

    /** A dielectric's reflection alone from outside, by ior, roughness and cosine. **/
    Array<const float> reflected;

    /** The reflection's directional albedo at a cosine and an alpha. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, then the alpha
    MOYA_KERNEL float at(float cosine, float alpha) const {
        if (cosines < 2U || roughnesses < 2U) {
            return 1.0F;
        }
        return grid(directional, 0U, place(cosine, cosines), place(std::sqrt(alpha), roughnesses));
    }

    /** The reflection's mean albedo at an alpha. **/
    MOYA_KERNEL float averageAt(float alpha) const {
        if (cosines < 2U || roughnesses < 2U) {
            return 1.0F;
        }
        return line(average, 0U, place(std::sqrt(alpha), roughnesses));
    }

    /** A dielectric's directional albedo at a cosine, an alpha and a ratio eta, from a side. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, the alpha, then eta
    MOYA_KERNEL float dielectricAt(float cosine, float alpha, float eta, bool inside) const {
        if (cosines < 2U || roughnesses < 2U || iors < 2U) {
            return 1.0F;
        }
        return volume(inside ? leaving : entering, cosine, alpha, eta);
    }

    /** A dielectric's reflection alone from outside, at a cosine, an alpha and a ratio eta. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, the alpha, then eta
    MOYA_KERNEL float reflectedAt(float cosine, float alpha, float eta) const {
        if (cosines < 2U || roughnesses < 2U || iors < 2U) {
            return 0.0F;
        }
        return volume(reflected, cosine, alpha, eta);
    }

    /** A dielectric's mean albedo at an alpha and a ratio eta, from a side. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the alpha, then eta
    MOYA_KERNEL float dielectricAverageAt(float alpha, float eta, bool inside) const {
        if (cosines < 2U || roughnesses < 2U || iors < 2U) {
            return 1.0F;
        }
        const Array<const float> & table = inside ? leavingAverage : enteringAverage;
        const Place up = place(std::sqrt(alpha), roughnesses);
        const Place over = place(axis(eta) / kMostIorAxis, iors);
        const float low = line(table, over.index * roughnesses, up);
        return low + ((line(table, (over.index + 1U) * roughnesses, up) - low) * over.fraction);
    }

    /** The mean share a smooth interface of a ratio eta transmits from outside. **/
    MOYA_KERNEL float transmittedAt(float eta) const {
        if (iors < 2U) {
            return 0.0F;
        }
        return line(transmitted, 0U, place(axis(eta) / kMostIorAxis, iors));
    }

    /** The mean share a thin slab of a ratio eta reflects. **/
    MOYA_KERNEL float slabAt(float eta) const {
        if (iors < 2U) {
            return 0.0F;
        }
        return line(slab, 0U, place(axis(eta) / kMostIorAxis, iors));
    }

    /** The ior axis's value for a ratio eta of at least one. **/
    MOYA_KERNEL static float axis(float eta) {
        return (eta - 1.0F) / (eta + 1.0F);
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

    /** A run of values from a start, read between two of them. **/
    MOYA_KERNEL static float line(const Array<const float> & table, std::uint32_t start, const Place & along) {
        const float low = table[start + along.index];
        return low + ((table[start + along.index + 1U] - low) * along.fraction);
    }

    /** A table by ior, roughness and cosine, read between eight of its values. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, the alpha, then eta
    MOYA_KERNEL float volume(const Array<const float> & table, float cosine, float alpha, float eta) const {
        const Place across = place(cosine, cosines);
        const Place up = place(std::sqrt(alpha), roughnesses);
        const Place over = place(axis(eta) / kMostIorAxis, iors);
        const std::uint32_t layer = cosines * roughnesses;
        const float low = grid(table, over.index * layer, across, up);
        return low + ((grid(table, (over.index + 1U) * layer, across, up) - low) * over.fraction);
    }

    /** A grid of rows of cosines from a start, read between four of its values. **/
    MOYA_KERNEL float grid(const Array<const float> & table, std::uint32_t start, const Place & across, const Place & up) const {
        const float low = line(table, start + (up.index * cosines), across);
        return low + ((line(table, start + ((up.index + 1U) * cosines), across) - low) * up.fraction);
    }
};

};  // namespace v3d::moya::kernel::bsdf

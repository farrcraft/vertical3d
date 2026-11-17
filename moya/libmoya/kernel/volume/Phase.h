/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/surface/Frame.h>

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::volume {

/** Pi, and one over four pi, the density of a direction drawn evenly over the sphere. **/
inline constexpr float kPhasePi = 3.14159265F;
inline constexpr float kQuarterOverPi = 1.0F / (4.0F * kPhasePi);

/** The anisotropy below which Henyey and Greenstein's phase function is drawn as an even one. **/
inline constexpr float kNearlyEven = 1.0e-3F;

/**
 * Henyey and Greenstein's phase function of an anisotropy, at the cosine between the direction a
 * path arrives along and the one it leaves along. An anisotropy of zero scatters evenly, one
 * above zero onwards along the path and one below zero back. It integrates to one over the
 * sphere, so it is also the density of a direction henyeyGreensteinSample() draws.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, then the anisotropy
MOYA_KERNEL inline float henyeyGreenstein(float cosine, float anisotropy) {
    const float squared = anisotropy * anisotropy;
    const float base = 1.0F + squared - (2.0F * anisotropy * cosine);
    return kQuarterOverPi * (1.0F - squared) / (base * std::sqrt(base));
}

/**
 * A direction drawn from Henyey and Greenstein's phase function of an anisotropy about the
 * direction a path arrives along, from two numbers in [0, 1). Its cosine to that direction is
 * drawn by inverting the function's distribution, and its turn about it evenly.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the direction, then the anisotropy
MOYA_KERNEL inline glm::vec3 henyeyGreensteinSample(const glm::vec3 & forward, float anisotropy, const glm::vec2 & u) {
    float cosine = 1.0F - (2.0F * u.x);
    if (std::fabs(anisotropy) >= kNearlyEven) {
        const float squared = anisotropy * anisotropy;
        const float term = (1.0F - squared) / (1.0F - anisotropy + (2.0F * anisotropy * u.x));
        cosine = (1.0F + squared - (term * term)) / (2.0F * anisotropy);
    }
    cosine = std::fmin(1.0F, std::fmax(-1.0F, cosine));
    const float across = std::sqrt(std::fmax(0.0F, 1.0F - (cosine * cosine)));
    const float turn = 2.0F * kPhasePi * u.y;
    const surface::Frame frame = surface::Frame::around(forward);
    return glm::normalize(frame.toWorld(glm::vec3 { across * std::cos(turn), across * std::sin(turn), cosine }));
}

};  // namespace v3d::moya::kernel::volume

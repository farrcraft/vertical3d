/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/kernel/spectrum/CieObserver.h>
#include <moya/libmoya/kernel/spectrum/Colorimetry.h>

#include <cmath>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::spectrum {

/** The temperatures, in kelvin, MaterialX's blackbody holds a temperature between. **/
inline constexpr float kBlackbodyLeast = 800.0F;
inline constexpr float kBlackbodyMost = 25000.0F;

/** The step, in nanometres, at which blackbody() sums a radiator through the observer. **/
inline constexpr int kBlackbodyStep = 5;

/**
 * The colour of a Planckian radiator at a temperature in kelvin, at a luminance of one, in ACEScg
 * with no chromatic adaptation, as a light's colour temperature is. Planck's law is summed through
 * the observer every kBlackbodyStep nanometres. The temperature is held to [kBlackbodyLeast,
 * kBlackbodyMost], as MaterialX's blackbody holds it, and a component below zero is zero.
 **/
MOYA_KERNEL inline glm::vec3 blackbody(float temperature) {
    const float kelvin = std::fmin(std::fmax(temperature, kBlackbodyLeast), kBlackbodyMost);
    glm::vec3 xyz { 0.0F };
    for (int row = 0; row < kObserverRows; row += kBlackbodyStep) {
        const float lambda = static_cast<float>(kObserverFirst + row);
        xyz += observer(lambda) * planck(lambda, kelvin);
    }
    if (!(xyz.y > 0.0F)) {
        return glm::vec3 { 1.0F };
    }
    const glm::vec3 out = acescgFromXyz(xyz / xyz.y);
    return glm::vec3 { std::fmax(out.x, 0.0F), std::fmax(out.y, 0.0F), std::fmax(out.z, 0.0F) };
}

};  // namespace v3d::moya::kernel::spectrum

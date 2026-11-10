/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Colorimetry.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/RgbTable.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::kernel::light {

/** A value held to [low, high]. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the value, then its bounds
MOYA_KERNEL inline float heldTo(float value, float low, float high) {
    if (value < low) {
        return low;
    }
    return value > high ? high : value;
}

/** How far from zero to one a value has come between two edges, eased at both ends. **/
MOYA_KERNEL inline float smoothStep(float value, float from, float to) {
    if (!(to > from)) {
        return value < from ? 0.0F : 1.0F;
    }
    const float along = heldTo((value - from) / (to - from), 0.0F, 1.0F);
    // Hermite's cubic, 3t^2 - 2t^3
    constexpr float kSquared = 3.0F;
    return along * along * (kSquared - (2.0F * along));
}

/**
 * The share of a light's radiance its cone lets out at an angle off its axis, whose cosine is
 * given: one inside the cone, falling smoothly to zero at its edge, and zero past it.
 **/
MOYA_KERNEL inline float coneShare(const Light & light, float cosine) {
    if (light.coneAngle >= kQuarterTurn && !(light.coneSoftness > 0.0F)) {
        return 1.0F;
    }
    const float off = std::acos(heldTo(cosine, -1.0F, 1.0F));
    if (off > light.coneAngle) {
        return 0.0F;
    }
    return 1.0F - smoothStep(off, light.coneAngle * (1.0F - light.coneSoftness), light.coneAngle);
}

/** A light's spectrum at the wavelengths a path carries, before its scale and its cone. **/
MOYA_KERNEL inline spectrum::SampledSpectrum spectrumOf(const Light & light, const spectrum::RgbTable & table, const spectrum::SampledWavelengths & wavelengths) {
    if (!(light.temperature > 0.0F)) {
        return spectrum::illuminant(table, light.colour, wavelengths);
    }
    spectrum::SampledSpectrum out = spectrum::unbounded(table, light.colour, wavelengths);
    for (int i = 0; i < spectrum::kSamples; ++i) {
#ifdef MOYA_RGB
        out[i] *= light.planckColour[i];
#else
        out[i] *= spectrum::planck(wavelengths.lambda[i], light.temperature) * light.planckScale;
#endif
    }
    return out;
}

/**
 * The normal of the face a light emits from, at a point of its shape in its prototype's space:
 * -z for a rect or a disk, and outwards for a sphere or a cylinder. It is not of unit length.
 **/
MOYA_KERNEL inline glm::vec3 emittingNormal(const Light & light, const glm::vec3 & local) {
    switch (light.shape) {
        case Shape::Rect:
        case Shape::Disk:
            break;
        case Shape::Sphere:
            return local;
        case Shape::Cylinder:
            return glm::vec3 { local.x, local.y, 0.0F };
    }
    return glm::vec3 { 0.0F, 0.0F, -1.0F };
}

/**
 * The radiance a light sends from a point of its shape in the world towards a unit direction,
 * with its prototype's transformation into the world. It sends none from behind its emitting
 * face. Its face and its cone's axis are turned into the world as normals are.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, then the direction
MOYA_KERNEL inline spectrum::SampledSpectrum radiance(const Light & light, const glm::mat4 & toWorld, const glm::vec3 & point, const glm::vec3 & towards,
    const spectrum::RgbTable & table, const spectrum::SampledWavelengths & wavelengths) {
    const glm::mat4 toLocal = glm::inverse(toWorld);
    const glm::mat3 normals = glm::transpose(glm::mat3(toLocal));
    const glm::vec3 local { toLocal * glm::vec4(point, 1.0F) };
    if (!(glm::dot(normals * emittingNormal(light, local), towards) > 0.0F)) {
        return spectrum::SampledSpectrum(0.0F);
    }
    const float share = coneShare(light, glm::dot(glm::normalize(normals * light.axis), towards));
    if (!(share > 0.0F)) {
        return spectrum::SampledSpectrum(0.0F);
    }
    return spectrumOf(light, table, wavelengths) * (light.scale * share);
}

};  // namespace v3d::moya::kernel::light

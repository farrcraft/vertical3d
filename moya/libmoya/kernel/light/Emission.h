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

/**
 * A light's spectrum at the wavelengths a path carries, before its scale and its cone, with its
 * colour tinted by a texture's.
 **/
MOYA_KERNEL inline spectrum::SampledSpectrum spectrumOf(const Light & light, const glm::vec3 & tint, const spectrum::RgbTable & table,
    const spectrum::SampledWavelengths & wavelengths) {
    const glm::vec3 colour = light.colour * tint;
    if (!(light.temperature > 0.0F)) {
        return spectrum::illuminant(table, colour, wavelengths);
    }
    spectrum::SampledSpectrum out = spectrum::unbounded(table, colour, wavelengths);
    for (int i = 0; i < spectrum::kSamples; ++i) {
#ifdef MOYA_RGB
        out[i] *= light.planckColour[i];
#else
        out[i] *= spectrum::planck(wavelengths.lambda[i], light.temperature) * light.planckScale;
#endif
    }
    return out;
}

/** A light's spectrum at the wavelengths a path carries, before its scale and its cone. **/
MOYA_KERNEL inline spectrum::SampledSpectrum spectrumOf(const Light & light, const spectrum::RgbTable & table, const spectrum::SampledWavelengths & wavelengths) {
    return spectrumOf(light, glm::vec3 { 1.0F }, table, wavelengths);
}

/**
 * The normal of the face a light emits from, at a point of its shape in its prototype's space:
 * -z for a rect or a disk, and outwards for a sphere or a cylinder. It is not of unit length. A
 * light with no shape has none, and gives -z.
 **/
MOYA_KERNEL inline glm::vec3 emittingNormal(const Light & light, const glm::vec3 & local) {
    switch (light.shape) {
        case Shape::Rect:
        case Shape::Disk:
        case Shape::Point:
        case Shape::Distant:
        case Shape::Mesh:
        case Shape::Dome:
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
 * with its prototype's transformation into the world, its colour tinted by its texture's there.
 * It sends none from behind its emitting face. Its face and its cone's axis are turned into the
 * world as normals are.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, then the direction
MOYA_KERNEL inline spectrum::SampledSpectrum radiance(const Light & light, const glm::mat4 & toWorld, const glm::vec3 & point, const glm::vec3 & towards,
    const glm::vec3 & tint, const spectrum::RgbTable & table, const spectrum::SampledWavelengths & wavelengths) {
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
    return spectrumOf(light, tint, table, wavelengths) * (light.scale * share);
}

/** The radiance a mesh light sends from the side its triangles emit from. No cone shapes it. **/
MOYA_KERNEL inline spectrum::SampledSpectrum glow(const Light & light, const spectrum::RgbTable & table, const spectrum::SampledWavelengths & wavelengths) {
    return spectrumOf(light, table, wavelengths) * light.scale;
}

/** The unit direction a light's axis points along in the world, turned as a normal is. **/
MOYA_KERNEL inline glm::vec3 axisOf(const Light & light, const glm::mat4 & toWorld) {
    return glm::normalize(glm::transpose(glm::inverse(glm::mat3(toWorld))) * light.axis);
}

/** One less the cosine of a distant light's half angle, as twice the sine squared of its half. **/
MOYA_KERNEL inline float distantRise(const Light & light) {
    const float sine = std::sin(0.5F * light.size.x);
    return 2.0F * sine * sine;
}

/**
 * What a distant light with an angle sends along a unit direction towards it: its radiance
 * inside its cone, and nothing outside it or for a light of no angle.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the transformation, then the direction
MOYA_KERNEL inline spectrum::SampledSpectrum fromAfar(const Light & light, const glm::mat4 & toWorld, const glm::vec3 & towards,
    const spectrum::RgbTable & table, const spectrum::SampledWavelengths & wavelengths) {
    if (light.shape != Shape::Distant || !(light.size.x > 0.0F) || !(glm::dot(-axisOf(light, toWorld), towards) >= 1.0F - distantRise(light))) {
        return spectrum::SampledSpectrum(0.0F);
    }
    return spectrumOf(light, table, wavelengths) * light.scale;
}

/**
 * What a light sends to a point from a point drawn on it, along the unit direction to it, its
 * colour tinted by its texture's at the point drawn. A light with a shape sends its radiance. A
 * point sends its radiant intensity, shaped by its cone, over the distance squared, and a distant
 * light of no angle its irradiance. A distant light with
 * an angle sends its radiance inside its cone.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point it reaches, then the point drawn
MOYA_KERNEL inline spectrum::SampledSpectrum arriving(const Light & light, const glm::mat4 & toWorld, const glm::vec3 & from, const glm::vec3 & drawn,
    const glm::vec3 & incoming, const glm::vec3 & tint, const spectrum::RgbTable & table, const spectrum::SampledWavelengths & wavelengths) {
    switch (light.shape) {
        case Shape::Point: {
            const glm::vec3 between = drawn - from;
            const float squared = glm::dot(between, between);
            const float share = coneShare(light, glm::dot(axisOf(light, toWorld), -incoming));
            if (!(squared > 0.0F) || !(share > 0.0F)) {
                return spectrum::SampledSpectrum(0.0F);
            }
            return spectrumOf(light, table, wavelengths) * (light.scale * share / squared);
        }
        case Shape::Distant:
            if (!(light.size.x > 0.0F)) {
                return spectrumOf(light, table, wavelengths) * light.scale;
            }
            return fromAfar(light, toWorld, incoming, table, wavelengths);
        case Shape::Rect:
        case Shape::Disk:
        case Shape::Sphere:
        case Shape::Cylinder:
        case Shape::Mesh:
        case Shape::Dome:
            break;
    }
    return radiance(light, toWorld, drawn, -incoming, tint, table, wavelengths);
}

};  // namespace v3d::moya::kernel::light

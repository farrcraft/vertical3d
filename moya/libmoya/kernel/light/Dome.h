/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/light/Emission.h>
#include <moya/libmoya/kernel/light/Image.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/LightSample.h>
#include <moya/libmoya/kernel/light/Lights.h>
#include <moya/libmoya/kernel/light/Sampling.h>
#include <moya/libmoya/kernel/light/SphericalRectangle.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/RgbTable.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>

#include <cmath>
#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::light {

/*
 * A dome light's image is laid out in latitude and longitude, as OpenEXR lays out an environment
 * and UsdLux takes it. Its top is the dome's +y and its bottom -y. Its middle across faces +z, and
 * a quarter of the way in from its left +x. The image is read as a texture over [0, 1] on each
 * axis, each texel the box it covers. The dome's transformation turns the image into the world.
 *
 * A texel is drawn as a row in proportion to the rows' sums, then a texel along it, then a place
 * in it uniformly. A point of the image covers 2 pi^2 times the cosine of its latitude of solid
 * angle for each unit of its area. Its density by solid angle is its density over the image over
 * that. A dome with no image is drawn uniformly over the sphere.
 */

/** Twice pi squared: the solid angle a unit of the image's area covers at its middle row. **/
inline constexpr float kTwicePiSquared = 19.7392088021787172F;

/** The direction in the dome's own space a place in its image faces, (0, 0) at its top left. **/
MOYA_KERNEL inline glm::vec3 domeDirection(const glm::vec2 & at) {
    const float longitude = kTurn * (0.5F - at.x);
    const float latitude = kHalfTurn * (0.5F - at.y);
    const float across = std::cos(latitude);
    return glm::vec3 { std::sin(longitude) * across, std::sin(latitude), std::cos(longitude) * across };
}

/** The place in a dome's image a unit direction in its own space faces. **/
MOYA_KERNEL inline glm::vec2 domePlace(const glm::vec3 & direction) {
    const float latitude = std::asin(std::fmin(1.0F, std::fmax(-1.0F, direction.y)));
    const float longitude = std::atan2(direction.x, direction.z);
    return glm::vec2 { 0.5F - (longitude / kTurn), 0.5F - (latitude / kHalfTurn) };
}

/** A unit direction in the world, in a dome's own space. **/
MOYA_KERNEL inline glm::vec3 domeLocal(const glm::mat4 & toWorld, const glm::vec3 & towards) {
    return glm::normalize(glm::inverse(glm::mat3(toWorld)) * towards);
}

/** The radiance the dome light at a place sends along a unit direction to it in the world. **/
MOYA_KERNEL inline spectrum::SampledSpectrum domeRadiance(const Lights & lights, std::uint32_t place, const glm::mat4 & toWorld, const glm::vec3 & towards,
    const spectrum::RgbTable & table, const spectrum::SampledWavelengths & wavelengths) {
    const Light & light = lights.lights[place];
    if (light.image == geometry::kNoPrimitive) {
        return spectrumOf(light, table, wavelengths) * light.scale;
    }
    const glm::vec3 tint = lights.texel(lights.images[light.image], domePlace(domeLocal(toWorld, towards)));
    return spectrumOf(light, tint, table, wavelengths) * light.scale;
}

/** The first row sum and the first column sum of the way an image is drawn by. **/
MOYA_KERNEL inline glm::uvec2 waysOf(const Image & image, bool compensated) {
    return compensated ? glm::uvec2 { image.compensatedRows, image.compensatedColumns } : glm::uvec2 { image.rows, image.columns };
}

/**
 * A direction to the dome light at a place, drawn from two random numbers, by the way its image is
 * drawn: compensated or not. Its point lies a unit along the direction.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point drawn from, then the numbers
MOYA_KERNEL inline LightSample domeSample(const Lights & lights, std::uint32_t place, const glm::mat4 & toWorld, const glm::vec3 & from, const glm::vec2 & u,
    bool compensated) {
    const Light & light = lights.lights[place];
    LightSample out;
    if (light.image == geometry::kNoPrimitive) {
        const float z = 1.0F - (2.0F * u.x);
        const float across = std::sqrt(std::fmax(0.0F, 1.0F - (z * z)));
        out.incoming = glm::vec3 { across * std::cos(kTurn * u.y), across * std::sin(kTurn * u.y), z };
        out.point = from + out.incoming;
        out.density = 1.0F / (2.0F * kTurn);
        return out;
    }
    const Image & image = lights.images[light.image];
    const glm::uvec2 ways = waysOf(image, compensated);
    const std::uint32_t row = Lights::drawnFrom(lights.sums, ways.x, image.height, u.y);
    const float rowChance = Lights::chanceIn(lights.sums, ways.x, image.height, row);
    const float rowBefore = row == 0U ? 0.0F : lights.sums[ways.x + row - 1U];
    const std::uint32_t columns = ways.y + (row * image.width);
    const std::uint32_t column = Lights::drawnFrom(lights.sums, columns, image.width, u.x);
    const float columnChance = Lights::chanceIn(lights.sums, columns, image.width, column);
    const float columnBefore = column == 0U ? 0.0F : lights.sums[columns + column - 1U];
    if (!(rowChance > 0.0F) || !(columnChance > 0.0F)) {
        return out;
    }
    // the random numbers' places within the row and the texel they drew, which are uniform there
    const float down = (static_cast<float>(row) + std::fmin(0.99999994F, (u.y - rowBefore) / rowChance)) / static_cast<float>(image.height);
    const float across = (static_cast<float>(column) + std::fmin(0.99999994F, (u.x - columnBefore) / columnChance)) / static_cast<float>(image.width);
    const glm::vec3 local = domeDirection(glm::vec2 { across, down });
    const float cosine = std::sqrt(std::fmax(0.0F, 1.0F - (local.y * local.y)));
    if (!(cosine > 0.0F)) {
        return out;
    }
    out.incoming = glm::normalize(glm::mat3(toWorld) * local);
    out.point = from + out.incoming;
    out.density = rowChance * columnChance * static_cast<float>(image.width * image.height) / (kTwicePiSquared * cosine);
    return out;
}

/**
 * The density by solid angle with which domeSample() draws a unit direction in the world, by the
 * way its image is drawn: compensated or not.
 **/
MOYA_KERNEL inline float domeDensity(const Lights & lights, std::uint32_t place, const glm::mat4 & toWorld, const glm::vec3 & towards, bool compensated) {
    const Light & light = lights.lights[place];
    if (light.image == geometry::kNoPrimitive) {
        return 1.0F / (2.0F * kTurn);
    }
    const Image & image = lights.images[light.image];
    const glm::vec3 local = domeLocal(toWorld, towards);
    const float cosine = std::sqrt(std::fmax(0.0F, 1.0F - (local.y * local.y)));
    if (!(cosine > 0.0F)) {
        return 0.0F;
    }
    const glm::vec2 at = domePlace(local);
    const glm::uvec2 ways = waysOf(image, compensated);
    const std::uint32_t row = Lights::cellOf(at.y, image.height);
    const std::uint32_t column = Lights::cellOf(at.x, image.width);
    const float chance = Lights::chanceIn(lights.sums, ways.x, image.height, row) * Lights::chanceIn(lights.sums, ways.y + (row * image.width), image.width, column);
    return chance * static_cast<float>(image.width * image.height) / (kTwicePiSquared * cosine);
}

};  // namespace v3d::moya::kernel::light

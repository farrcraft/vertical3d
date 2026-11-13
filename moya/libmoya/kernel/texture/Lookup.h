/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/surface/TextureFootprint.h>
#include <moya/libmoya/kernel/texture/Address.h>
#include <moya/libmoya/kernel/texture/Filtering.h>
#include <moya/libmoya/kernel/texture/Image.h>
#include <moya/libmoya/kernel/texture/Sampling.h>
#include <moya/libmoya/kernel/texture/Texels.h>
#include <moya/libmoya/kernel/texture/Texture.h>

#include <cmath>
#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::kernel::texture {

/** The most probes a lookup takes along its footprint's major axis. **/
inline constexpr std::uint32_t kMostProbes = 8;

/**
 * The farthest a texel index lies from zero before it is held there. A coordinate that far out has
 * lost the precision to name a texel, and an index stays within what its type holds.
 **/
inline constexpr float kFarthestIndex = 1099511627776.0F;

/** A whole number of texels as an index, held within kFarthestIndex of zero. **/
MOYA_KERNEL inline std::int64_t indexOf(float whole) {
    const float held = std::fmin(std::fmax(whole, -kFarthestIndex), kFarthestIndex);
    return static_cast<std::int64_t>(held);  // checked: held within kFarthestIndex, which fmax gives a NaN
}

/** The texel an index along an axis of a size reads by an address mode. Constant clamps. **/
MOYA_KERNEL inline std::uint32_t addressed(std::int64_t index, std::uint32_t size, Address mode) {
    const auto count = static_cast<std::int64_t>(size);
    if (index >= 0 && index < count) {
        return static_cast<std::uint32_t>(index);
    }
    switch (mode) {
        case Address::Periodic:
            return static_cast<std::uint32_t>(((index % count) + count) % count);
        case Address::Mirror: {
            const std::int64_t twice = count * 2;
            const std::int64_t folded = ((index % twice) + twice) % twice;
            return static_cast<std::uint32_t>(folded < count ? folded : twice - 1 - folded);
        }
        case Address::Constant:
        case Address::Clamp:
            break;
    }
    return index < 0 ? 0U : size - 1U;
}

/**
 * A level's texels about a place, weighed bilinearly. The place is across the texture from its
 * left and down it from its top, from zero to one.
 **/
template <Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the texture, then its level
MOYA_KERNEL glm::vec4 bilinear(const X & texels, std::uint32_t texture, std::uint32_t level, const glm::vec2 & place, const Sampling & sampling) {
    const Texture & held = texels.catalog.textures[texture];
    const std::uint32_t width = held.widthAt(level);
    const std::uint32_t height = held.heightAt(level);
    const float x = (place.x * static_cast<float>(width)) - 0.5F;
    const float y = (place.y * static_cast<float>(height)) - 0.5F;
    const float left = std::floor(x);
    const float top = std::floor(y);
    const float across = x - left;
    const float down = y - top;
    const std::int64_t column = indexOf(left);
    const std::int64_t row = indexOf(top);
    const std::uint32_t c0 = addressed(column, width, sampling.s);
    const std::uint32_t c1 = addressed(column + 1, width, sampling.s);
    const std::uint32_t r0 = addressed(row, height, sampling.t);
    const std::uint32_t r1 = addressed(row + 1, height, sampling.t);
    const glm::vec4 upper = (texels.texel(texture, level, c0, r0) * (1.0F - across)) + (texels.texel(texture, level, c1, r0) * across);
    const glm::vec4 lower = (texels.texel(texture, level, c0, r1) * (1.0F - across)) + (texels.texel(texture, level, c1, r1) * across);
    return (upper * (1.0F - down)) + (lower * down);
}

/** Two levels about a place, each bilinear, mixed by the fraction of a level between them. **/
template <Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the texture, then the level read
MOYA_KERNEL glm::vec4 trilinear(const X & texels, std::uint32_t texture, float level, const glm::vec2 & place, const Sampling & sampling) {
    const Texture & held = texels.catalog.textures[texture];
    const float last = static_cast<float>(held.levels - 1U);
    const float clamped = std::fmin(std::fmax(level, 0.0F), last);
    const float lower = std::floor(clamped);
    const float fraction = clamped - lower;
    const auto first = static_cast<std::uint32_t>(lower);  // checked: held within the texture's levels
    const glm::vec4 near = bilinear(texels, texture, first, place, sampling);
    if (!(fraction > 0.0F)) {
        return near;
    }
    return (near * (1.0F - fraction)) + (bilinear(texels, texture, first + 1U, place, sampling) * fraction);
}

/**
 * One texture over a footprint. The footprint's axes are changes in u and v. Its minor axis's
 * length in texels of the first level chooses the level, and probes spread along its major
 * axis, as many as the axes' ratio and at most kMostProbes. A footprint longer than that is read
 * at the coarser level its major axis over kMostProbes gives.
 **/
template <Texels X>
MOYA_KERNEL glm::vec4 filtered(const X & texels, std::uint32_t texture, const glm::vec2 & place, const surface::TextureFootprint & footprint,
    const Sampling & sampling) {
    const Texture & held = texels.catalog.textures[texture];
    const glm::vec2 size { static_cast<float>(held.width), static_cast<float>(held.height) };
    // a change in v moves up the texture, and a place runs down it
    const glm::vec2 major { footprint.major.x, -footprint.major.y };
    float majorLength = glm::length(major * size);
    float minorLength = glm::length(glm::vec2 { footprint.minor.x, -footprint.minor.y } * size);
    if (!(majorLength >= minorLength)) {
        majorLength = minorLength;
    }
    if (!std::isfinite(majorLength)) {
        // a footprint without bound, or of no number, covers the whole texture
        return trilinear(texels, texture, static_cast<float>(held.levels), place, sampling);
    }
    if (!(minorLength * static_cast<float>(kMostProbes) >= majorLength)) {
        minorLength = majorLength / static_cast<float>(kMostProbes);
    }
    const float level = minorLength > 1.0F ? std::log2(minorLength) : 0.0F;
    std::uint32_t probes = 1U;
    if (minorLength > 0.0F) {
        // the major axis is never the shorter, so the ratio is at least one
        const float ratio = std::fmin(majorLength / minorLength, static_cast<float>(kMostProbes));
        probes = static_cast<std::uint32_t>(std::ceil(ratio));  // checked: from one to kMostProbes
    }
    glm::vec4 sum { 0.0F };
    for (std::uint32_t i = 0; i < probes; ++i) {
        const float along = ((static_cast<float>(i) + 0.5F) / static_cast<float>(probes)) - 0.5F;
        sum += trilinear(texels, texture, level, place + (major * along), sampling);
    }
    return sum / static_cast<float>(probes);
}

/**
 * An image read at a texture coordinate over a footprint, or the fallback where it reads nothing.
 *
 * A UDIM image reads the tile its coordinate's whole parts name, at the coordinate's fractions,
 * and a coordinate outside its grid or on a missing tile gives the fallback. An address mode of
 * Constant gives the fallback for a coordinate outside zero to one along its axis. v runs up the
 * texture, so a v of zero reads its bottom row. Closest reads the first level's texel the
 * coordinate lies in.
 **/
template <Texels X>
MOYA_KERNEL glm::vec4 lookup(const X & texels, std::uint32_t image, glm::vec2 uv, const surface::TextureFootprint & footprint, const Sampling & sampling,
    const glm::vec4 & fallback) {
    if (image >= texels.catalog.images.size || !std::isfinite(uv.x) || !std::isfinite(uv.y)) {
        return fallback;
    }
    const Image & grid = texels.catalog.images[image];
    std::uint32_t tile = 0;
    if (grid.columns > 1U || grid.rows > 1U) {
        const float column = std::floor(uv.x);
        const float row = std::floor(uv.y);
        if (column < 0.0F || row < 0.0F || column >= static_cast<float>(grid.columns) || row >= static_cast<float>(grid.rows)) {
            return fallback;
        }
        // checked: both lie within the grid
        tile = (static_cast<std::uint32_t>(row) * grid.columns) + static_cast<std::uint32_t>(column);
        uv -= glm::vec2 { column, row };
    }
    const std::uint32_t texture = texels.catalog.tiles[grid.first + tile];
    if (texture == kNoTexture) {
        return fallback;
    }
    const bool outsideU = uv.x < 0.0F || uv.x > 1.0F;
    const bool outsideV = uv.y < 0.0F || uv.y > 1.0F;
    if ((sampling.s == Address::Constant && outsideU) || (sampling.t == Address::Constant && outsideV)) {
        return fallback;
    }
    const glm::vec2 place { uv.x, 1.0F - uv.y };
    if (sampling.filtering == Filtering::Closest) {
        const Texture & held = texels.catalog.textures[texture];
        const std::int64_t column = indexOf(std::floor(place.x * static_cast<float>(held.width)));
        const std::int64_t row = indexOf(std::floor(place.y * static_cast<float>(held.height)));
        return texels.texel(texture, 0U, addressed(column, held.width, sampling.s), addressed(row, held.height, sampling.t));
    }
    return filtered(texels, texture, place, footprint, sampling);
}

};  // namespace v3d::moya::kernel::texture

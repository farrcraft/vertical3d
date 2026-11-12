/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

/** The integrator: paths built in stages over an explicit state, and the settings they follow. **/
namespace v3d::moya::kernel::integrate {

/*
 * Where each draw of a path takes its dimension of the sampler. The camera's draws come first, then
 * a block for each bounce, and each draw has a fixed place in its block. Any stage can then draw
 * its numbers from the pixel, the sample, the bounce and the place alone.
 */

/** The camera's draws: the filter's offset, the lens point, the time and the wavelengths. **/
inline constexpr std::uint32_t kFilter = 0;
inline constexpr std::uint32_t kLens = 1;
inline constexpr std::uint32_t kTime = 2;
inline constexpr std::uint32_t kWavelengths = 3;
inline constexpr std::uint32_t kCameraDraws = 4;

/**
 * A bounce's draws. They are the BSDF's direction, whether Russian roulette ends the path, the
 * light it samples, the point on that light, and the triangle of a mesh light drawn by power. The
 * last is the BSDF's choice between reflecting and transmitting. The light tree draws a mesh
 * light's triangle from the light's own random number.
 **/
inline constexpr std::uint32_t kScatter = 0;
inline constexpr std::uint32_t kRoulette = 1;
inline constexpr std::uint32_t kLightPick = 2;
inline constexpr std::uint32_t kLightPoint = 3;
inline constexpr std::uint32_t kLightTriangle = 4;
inline constexpr std::uint32_t kScatterChoice = 5;

/** The places each bounce's block holds. **/
inline constexpr std::uint32_t kBounceDraws = 6;

/** The dimension of a draw at a place in a bounce's block. **/
MOYA_KERNEL constexpr std::uint32_t bounceDimension(std::uint32_t bounce, std::uint32_t place) {
    return kCameraDraws + (bounce * kBounceDraws) + place;
}

};  // namespace v3d::moya::kernel::integrate

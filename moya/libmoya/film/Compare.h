/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/film/Estimate.h>
#include <moya/libmoya/film/Snapshot.h>

#include <cstdint>
#include <functional>

#include <glm/vec2.hpp>

namespace v3d::moya::film {

/** A pixel's beauty as a luminance, with the snapshot's standard error for it. **/
Estimate luminance(const Snapshot & snapshot, std::uint32_t x, std::uint32_t y);

/**
 * The mean luminance of the pixels from one corner up to, but not including, the other, with its
 * standard error. The pixels are taken as independent estimates.
 *
 * @throws std::invalid_argument when the region holds no pixel or reaches outside the snapshot
 **/
Estimate mean(const Snapshot & snapshot, const glm::uvec2 & from, const glm::uvec2 & to);

/**
 * How many standard errors an estimate lies from an expected value. An estimate whose error is
 * infinite measures nothing, and lies 0 from any value. An error below a ten-thousandth of the
 * larger of 1 and the expected value is taken as that, so that rounding alone is never a
 * difference. A render of no noise, such as an RGB build's of a pixel whose samples agree, has an
 * error of 0 and still rounds by tens of millionths.
 **/
float sigmas(const Estimate & estimate, float expected);

/** How many standard errors two estimates lie apart, by their errors combined. **/
float sigmas(const Estimate & left, const Estimate & right);

/** The pixel of a comparison furthest from what it was compared with. **/
struct Furthest {
    glm::uvec2 pixel { 0 };

    /** How far it lies, in standard errors. A pixel that is not a number makes it NaN. **/
    float sigmas { 0.0F };

    /** The pixels whose error is infinite, which the comparison could not measure. **/
    std::uint32_t unmeasured { 0 };
};

/** The pixel whose luminance lies furthest from the value expected there. **/
Furthest furthest(const Snapshot & snapshot, const std::function<float(const glm::uvec2 &)> & expected);

/**
 * The pixel at which two snapshots' luminances lie furthest apart.
 *
 * @throws std::invalid_argument when their resolutions differ
 **/
Furthest furthest(const Snapshot & left, const Snapshot & right);

};  // namespace v3d::moya::film

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

#include <glm/ext/quaternion_common.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::kernel::geometry {

/**
 * One sample of a transformation, decomposed into a translation, a rotation and a symmetric
 * scale, which holds any shear. The transformation is the translation, then the rotation, then the
 * scale, applied to a point from the right.
 **/
struct Key {
    float time { 0.0F };
    glm::vec3 translation { 0.0F };
    glm::quat rotation { 1.0F, 0.0F, 0.0F, 0.0F };
    glm::mat3 scale { 1.0F };
};

/**
 * Where a time falls among samples: the sample at or before it, and how far it lies towards the
 * next, from 0 to 1. A time before the first sample is at the first, and one after the last is at
 * the last.
 **/
struct Between {
    std::uint32_t at { 0 };
    float along { 0.0F };
};

/**
 * Where a geometry's points are held: count points for each of its samples, one sample after
 * another from first, at the times from times on. A geometry of one sample holds it at every time.
 **/
struct PointSamples {
    std::uint32_t first { 0 };
    std::uint32_t count { 0 };
    std::uint32_t times { 0 };
    std::uint32_t samples { 1 };
};

/** Where a time falls among count rising times, of which time(i) gives the i-th. **/
template <typename Time>
MOYA_KERNEL Between between(const Time & time, std::uint32_t count, float t) {
    Between out;
    if (count < 2 || !(t > time(0U))) {
        return out;
    }
    if (t >= time(count - 1U)) {
        out.at = count - 1U;
        return out;
    }
    while (out.at + 2U < count && t >= time(out.at + 1U)) {
        ++out.at;
    }
    const float from = time(out.at);
    out.along = (t - from) / (time(out.at + 1U) - from);
    return out;
}

/** The transformation a key holds. **/
MOYA_KERNEL inline glm::mat4 compose(const Key & key) {
    glm::mat4 out(glm::mat3_cast(key.rotation) * key.scale);
    out[3] = glm::vec4(key.translation, 1.0F);
    return out;
}

/**
 * A transformation at a time, between the two of count keys that bracket it. The translation and
 * the scale are interpolated linearly, and the rotation the shortest way at an even speed.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - how many keys, then the time
MOYA_KERNEL inline glm::mat4 transformAt(const Key * keys, std::uint32_t count, float time) {
    const Between at = between(
        [&](std::uint32_t sample) {
            return keys[sample].time;
        },
        count, time);
    const Key & from = keys[at.at];
    if (at.along == 0.0F) {
        return compose(from);
    }
    const Key & to = keys[at.at + 1U];
    Key mixed;
    mixed.translation = from.translation + ((to.translation - from.translation) * at.along);
    mixed.rotation = glm::slerp(from.rotation, to.rotation, at.along);
    mixed.scale = from.scale + ((to.scale - from.scale) * at.along);
    return compose(mixed);
}

/** A geometry's point at a time, interpolated linearly between the samples that bracket it. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, then the time
MOYA_KERNEL inline glm::vec3 pointAt(const PointSamples & held, const glm::vec3 * points, const float * times, std::uint32_t index, float time) {
    const Between at = between(
        [&](std::uint32_t sample) {
            return times[held.times + sample];
        },
        held.samples, time);
    const glm::vec3 & from = points[held.first + (at.at * held.count) + index];
    if (at.along == 0.0F) {
        return from;
    }
    const glm::vec3 & to = points[held.first + ((at.at + 1U) * held.count) + index];
    return from + ((to - from) * at.along);
}

};  // namespace v3d::moya::kernel::geometry

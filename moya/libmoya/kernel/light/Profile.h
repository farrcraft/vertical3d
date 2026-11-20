/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/light/Emission.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>
#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::light {

/**
 * An IES profile that shapes a light: its candela by direction, in IES's type C photometry. Its
 * values are held among the lights' photometry. There are verticalCount vertical angles from
 * vertical, then horizontalCount horizontal angles from horizontal, both in radians and rising.
 * The candela follow from candela, all the vertical angles of the first horizontal angle, then of
 * the next.
 *
 * - A vertical angle is measured from the light's -z axis, its nadir, towards its +z.
 * - A horizontal angle is measured about that axis from the light's +x towards its +y. The
 *   horizontal angles span one angle, a quarter turn from 0, a half turn from 0, the half turn
 *   from a quarter turn to three quarters, or a whole turn from 0. A smaller span holds one part
 *   of a profile that is the same in each mirror of it.
 * - scale is what the candela is multiplied by, and angleScale UsdLux's ies:angleScale.
 **/
struct Profile {
    std::uint32_t vertical { 0 };
    std::uint32_t verticalCount { 0 };
    std::uint32_t horizontal { 0 };
    std::uint32_t horizontalCount { 0 };
    std::uint32_t candela { 0 };
    float scale { 1.0F };
    float angleScale { 0.0F };
};

/**
 * Where a value lies among count rising values from a first: the place of the segment that holds
 * it and how far along it lies. False outside them. One value holds only itself.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the first, then how many
MOYA_KERNEL inline bool segmentOf(const Array<float> & values, std::uint32_t first, std::uint32_t count, float value, std::uint32_t & place,
    float & along) {  // NOLINT(bugprone-easily-swappable-parameters)
    if (count == 0U || value < values[first] || value > values[first + count - 1U]) {
        return false;
    }
    place = 0;
    along = 0.0F;
    if (count == 1U) {
        return true;
    }
    std::uint32_t low = 0;
    std::uint32_t high = count - 1U;
    while (high - low > 1U) {
        const std::uint32_t middle = low + ((high - low) / 2U);
        if (value < values[first + middle]) {
            high = middle;
        } else {
            low = middle;
        }
    }
    const float width = values[first + high] - values[first + low];
    place = low;
    along = width > 0.0F ? (value - values[first + low]) / width : 0.0F;
    return true;
}

/** A horizontal angle in [0, 2 pi) folded into the span a profile's horizontal angles hold. **/
MOYA_KERNEL inline float folded(const Profile & profile, const Array<float> & values, float phi) {
    if (profile.horizontalCount < 2U) {
        return values[profile.horizontal];
    }
    const float last = values[profile.horizontal + profile.horizontalCount - 1U];
    constexpr float kSlack = 1.0e-4F;
    constexpr float kQuarter = 0.5F * kHalfTurn;
    constexpr float kThreeQuarters = kHalfTurn + kQuarter;
    constexpr float kWhole = 2.0F * kHalfTurn;
    if (values[profile.horizontal] > kQuarter - kSlack) {
        // a half turn from a quarter to three quarters, mirrored across it
        if (phi < kQuarter) {
            return kHalfTurn - phi;
        }
        return phi > kThreeQuarters ? (kWhole + kHalfTurn) - phi : phi;
    }
    if (last < kQuarter + kSlack) {
        // a quarter turn, mirrored across both its edges
        const float half = phi > kHalfTurn ? kWhole - phi : phi;
        return half > kQuarter ? kHalfTurn - half : half;
    }
    if (last < kHalfTurn + kSlack) {
        return phi > kHalfTurn ? (2.0F * kHalfTurn) - phi : phi;
    }
    return phi;
}

/** The candela of a profile at one of its horizontal angles and a vertical angle. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the horizontal place, then the angle
MOYA_KERNEL inline float candelaAt(const Profile & profile, const Array<float> & values, std::uint32_t horizontal, float theta) {
    std::uint32_t place = 0;
    float along = 0.0F;
    if (!segmentOf(values, profile.vertical, profile.verticalCount, theta, place, along)) {
        return 0.0F;
    }
    const std::uint32_t row = profile.candela + (horizontal * profile.verticalCount);
    const float low = values[row + place];
    return along > 0.0F ? low + ((values[row + place + 1U] - low) * along) : low;
}

/**
 * What a profile multiplies a light's emission by along a unit direction in the light's own
 * space, interpolated linearly between its angles. A vertical angle the profile does not reach
 * gives nothing. The angle scale applies first: above zero, the vertical angle is divided by it
 * from the nadir, and below, from the zenith, by its negation.
 **/
MOYA_KERNEL inline float profileShare(const Profile & profile, const Array<float> & values, const glm::vec3 & direction) {
    float theta = std::acos(heldTo(-direction.z, -1.0F, 1.0F));
    float phi = std::atan2(direction.y, direction.x);
    if (phi < 0.0F) {
        phi += 2.0F * kHalfTurn;
    }
    if (profile.angleScale > 0.0F) {
        theta /= profile.angleScale;
    } else if (profile.angleScale < 0.0F) {
        theta = ((theta - kHalfTurn) / -profile.angleScale) + kHalfTurn;
    }
    if (theta < 0.0F || theta > kHalfTurn) {
        return 0.0F;
    }
    std::uint32_t place = 0;
    float along = 0.0F;
    if (!segmentOf(values, profile.horizontal, profile.horizontalCount, folded(profile, values, phi), place, along)) {
        return 0.0F;
    }
    const float low = candelaAt(profile, values, place, theta);
    const float candela = along > 0.0F ? low + ((candelaAt(profile, values, place + 1U, theta) - low) * along) : low;
    return candela * profile.scale;
}

/**
 * A direction in a light's prototype space, in the light's own space. A cylinder light's shape is
 * turned from z onto x, so its prototype's x is its own -z.
 **/
MOYA_KERNEL inline glm::vec3 ownSpace(const Light & light, const glm::vec3 & local) {
    return light.shape == Shape::Cylinder ? glm::vec3 { local.z, local.y, -local.x } : local;
}

};  // namespace v3d::moya::kernel::light

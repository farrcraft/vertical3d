/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Clip.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <vector>

#include <glm/gtc/quaternion.hpp>

#include "Channel.h"
#include "Pose.h"

namespace v3d::animation {

namespace {

/**
 * The value of key `key` - the middle of three for a cubic spline.
 **/
glm::vec4 valueAt(const Channel& channel, std::size_t key) {
    return channel.interpolation == Channel::Interpolation::CubicSpline ? channel.values[(key * 3) + 1] : channel.values[key];
}

glm::quat asQuat(const glm::vec4& value) {
    // stored x, y, z, w, and glm's constructor takes w first
    return glm::quat(value.w, value.x, value.y, value.z);
}

/** The key a segment starts at, how far through it to sample, and how long it lasts. **/
struct Segment {
    std::size_t key = 0;
    float u = 0.0F;
    float span = 0.0F;
};

/**
 * Hermite between two keys, as glTF's cubic spline defines it: each key's value and its out or
 * in tangent, the tangents scaled by the time between the keys.
 **/
glm::vec4 hermite(const Channel& channel, const Segment& segment) {
    const std::size_t key = segment.key;
    const float u = segment.u;
    const float span = segment.span;
    const float u2 = u * u;
    const float u3 = u2 * u;
    const glm::vec4 from = channel.values[(key * 3) + 1];
    const glm::vec4 out = channel.values[(key * 3) + 2] * span;
    const glm::vec4 in = channel.values[(key + 1) * 3] * span;
    const glm::vec4 to = channel.values[((key + 1) * 3) + 1];
    // the cubic Hermite basis functions
    const float fromWeight = (2.0F * u3) - (3.0F * u2) + 1.0F;
    const float outWeight = u3 - (2.0F * u2) + u;
    const float toWeight = (-2.0F * u3) + (3.0F * u2);
    const float inWeight = u3 - u2;
    return fromWeight * from + outWeight * out + toWeight * to + inWeight * in;
}

/**
 * Set one joint's path of a pose from a value.
 **/
void apply(const Channel& channel, const glm::vec4& value, Pose::Joint* joint) {
    switch (channel.path) {
        case Channel::Path::Translation:
            joint->translation = glm::vec3(value);
            break;
        case Channel::Path::Rotation:
            joint->rotation = glm::normalize(asQuat(value));
            break;
        case Channel::Path::Scale:
            joint->scale = glm::vec3(value);
            break;
    }
}

/**
 * Whether a channel holds enough values for its keys. Sampling skips a channel that does not,
 * so it never reads past the values.
 **/
bool wellFormed(const Channel& channel) {
    const std::size_t per = channel.interpolation == Channel::Interpolation::CubicSpline ? 3 : 1;
    return !channel.times.empty() && channel.values.size() >= channel.times.size() * per;
}

void sampleChannel(const Channel& channel, float time, Pose::Joint* joint) {
    const std::vector<float>& times = channel.times;
    // a time that is not finite would find no key around it, so it reads as the first key
    if (!std::isfinite(time) || time <= times.front()) {
        apply(channel, valueAt(channel, 0), joint);
        return;
    }
    if (time >= times.back()) {
        apply(channel, valueAt(channel, times.size() - 1), joint);
        return;
    }

    // the key at or before the time, and the one after it
    const std::size_t key = static_cast<std::size_t>(std::distance(times.begin(), std::upper_bound(times.begin(), times.end(), time))) - 1;
    const float span = times[key + 1] - times[key];
    const float u = span > 0.0F ? (time - times[key]) / span : 0.0F;

    switch (channel.interpolation) {
        case Channel::Interpolation::Step:
            apply(channel, valueAt(channel, key), joint);
            break;
        case Channel::Interpolation::Linear:
            if (channel.path == Channel::Path::Rotation) {
                joint->rotation = glm::normalize(glm::slerp(asQuat(channel.values[key]), asQuat(channel.values[key + 1]), u));
            } else {
                apply(channel, glm::mix(channel.values[key], channel.values[key + 1], u), joint);
            }
            break;
        case Channel::Interpolation::CubicSpline:
            apply(channel, hermite(channel, {.key = key, .u = u, .span = span}), joint);
            break;
    }
}

};  // namespace

void sample(const Clip& clip, float time, Pose* pose) {
    for (const Channel& channel : clip.channels) {
        if (channel.joint >= pose->joints.size() || !wellFormed(channel)) {
            continue;
        }
        sampleChannel(channel, time, &pose->joints[channel.joint]);
    }
}

};  // namespace v3d::animation

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/vec4.hpp>

namespace v3d::type::animation {

struct Pose;

/**
 * One joint's translation, rotation or scale, keyed in time - glTF's animation channel, kept as
 * the file gave it.
 **/
struct Channel final {
    enum class Path {
        Translation,
        Rotation,
        Scale
    };

    enum class Interpolation {
        Step,         /**< each key held until the next **/
        Linear,       /**< lerped, or slerped the short way round for a rotation **/
        CubicSpline   /**< Hermite, with an in and an out tangent a key **/
    };

    uint16_t joint{ 0 };  /**< into the skeleton's joints **/
    Path path{ Path::Translation };
    Interpolation interpolation{ Interpolation::Linear };

    /**
     * The key times in seconds, rising.
     **/
    std::vector<float> times;

    /**
     * A value a key, or for a cubic spline three - its in tangent, its value and its out
     * tangent. A translation or a scale is xyz, and a rotation a quaternion in x, y, z, w order.
     **/
    std::vector<glm::vec4> values;
};

/**
 * A named set of channels, which sampled at a time gives a pose - ADR-0070.
 **/
struct Clip final {
    std::string name;
    float duration{ 0.0f };  /**< the last key of any channel **/
    std::vector<Channel> channels;
};

/**
 * Sample a clip, overwriting the joints it animates and leaving every other one as it is.
 *
 * A time before a channel's first key holds that key, and one after its last holds the last.
 *
 * @param time a time within the clip, which Clock::sample() gives from a playback's
 **/
void sample(const Clip& clip, float time, Pose* pose);

};  // namespace v3d::type::animation

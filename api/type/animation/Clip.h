/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Channel.h"

#include <string>
#include <vector>

namespace v3d::type::animation {

struct Pose;

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

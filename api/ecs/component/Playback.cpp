/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Playback.h"

#include <api/type/animation/Clock.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <glm/common.hpp>

namespace v3d::ecs::component {

namespace {

/**
 * How long a looping clip's unwrapped time may grow before whole loops are taken off it. A
 * float still resolves half a millisecond here, and loses that within the next few hours.
 **/
const float rebaseAfter = 4096.0f;

/**
 * The earlier of two times of the same looping clip, moved back by whole loops when the later
 * one was rebased in between, so that the two still run forwards.
 **/
float unrolled(const Playback& earlier, const Playback& later) {
    if (!later.loops || later.duration <= 0.0f || earlier.time <= later.time) {
        return earlier.time;
    }
    const float loops = std::ceil((earlier.time - later.time) / later.duration);
    return earlier.time - loops * later.duration;
}

};  // namespace

void play(Playback* playback, uint32_t clip, float duration, bool loops, float fade) {
    if (playback->clip == clip) {
        return;
    }
    const bool fading = fade > 0.0f && playback->clip != Playback::none;
    playback->from = fading ? playback->clip : Playback::none;
    playback->fromTime = playback->time;
    playback->fromDuration = playback->duration;
    playback->fromLoops = playback->loops;
    playback->fade = fading ? 0.0f : 1.0f;
    playback->fadeDuration = fading ? fade : 0.0f;

    playback->clip = clip;
    playback->time = 0.0f;
    playback->duration = duration;
    playback->loops = loops;
}

void advance(Playback* playback, float step) {
    if (playback->clip == Playback::none) {
        return;
    }
    const type::animation::Clock clock(playback->duration, playback->loops);
    playback->time = clock.advance(playback->time, step);
    if (playback->loops && playback->time >= rebaseAfter) {
        playback->time = clock.sample(playback->time);
    }

    if (playback->from == Playback::none) {
        return;
    }
    const type::animation::Clock fading(playback->fromDuration, playback->fromLoops);
    playback->fromTime = fading.sample(fading.advance(playback->fromTime, step));
    playback->fade = playback->fadeDuration > 0.0f ? std::min(1.0f, playback->fade + step / playback->fadeDuration) : 1.0f;
    if (playback->fade >= 1.0f) {
        playback->from = Playback::none;
    }
}

void advance(entt::registry& registry, float step) {
    for (auto [entity, playback] : registry.view<Playback>().each()) {
        advance(&playback, step);
    }
}

uint32_t crossed(const Playback& previous, const Playback& current, float marker) {
    if (previous.clip != current.clip || current.clip == Playback::none) {
        return 0;
    }
    const type::animation::Clock clock(current.duration, current.loops);
    return clock.crossed(unrolled(previous, current), current.time, marker);
}

Playback interpolate(const Playback& from, const Playback& to, float alpha) {
    if (from.clip != to.clip) {
        return to;
    }
    Playback blended = to;
    blended.time = glm::mix(unrolled(from, to), to.time, alpha);
    if (to.from != Playback::none && from.from == to.from) {
        // the faded clip's time is kept wrapped, so a wrap between the steps is drawn at the
        // later one rather than sweeping back through the clip
        blended.fromTime = from.fromTime <= to.fromTime ? glm::mix(from.fromTime, to.fromTime, alpha) : to.fromTime;
        blended.fade = glm::mix(from.fade, to.fade, alpha);
    }
    return blended;
}

};  // namespace v3d::ecs::component

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Playback.h"

#include <api/animation/Clock.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <glm/common.hpp>

namespace v3d::ecs::component {

namespace {

/**
 * The unwrapped time, in seconds, at which a looping clip's time is wrapped back into the clip.
 * A float near this value resolves about half a millisecond. Within a few more hours of play it
 * would resolve much less.
 **/
const float rebaseAfter = 4096.0F;

/**
 * earlier.time, moved back by whole loops when later.time was wrapped between the two steps.
 * The two times then still run forwards.
 **/
float unrolled(const Playback& earlier, const Playback& later) {
    if (!later.loops || later.duration <= 0.0F || earlier.time <= later.time) {
        return earlier.time;
    }
    const float loops = std::ceil((earlier.time - later.time) / later.duration);
    return earlier.time - (loops * later.duration);
}

};  // namespace

void play(Playback* playback, const Start& start) {
    const uint32_t clip = start.clip;
    const float duration = start.duration;
    const bool loops = start.loops;
    const float fade = start.fade;
    if (playback->clip == clip) {
        return;
    }
    const bool fading = fade > 0.0F && playback->clip != Playback::none;
    playback->from = fading ? playback->clip : Playback::none;
    playback->fromTime = playback->time;
    playback->fromDuration = playback->duration;
    playback->fromLoops = playback->loops;
    playback->fade = fading ? 0.0F : 1.0F;
    playback->fadeDuration = fading ? fade : 0.0F;

    playback->clip = clip;
    playback->time = 0.0F;
    playback->duration = duration;
    playback->loops = loops;
}

void advance(Playback* playback, float step) {
    // the rest pose has no clock, so only a clip's time advances here. A fade into the rest
    // pose still runs below. Without it, the old clip would never fade out
    if (playback->clip != Playback::none) {
        const animation::Clock clock(playback->duration, playback->loops);
        playback->time = clock.advance(playback->time, step);
        if (playback->loops && playback->time >= rebaseAfter) {
            playback->time = clock.sample(playback->time);
        }
    }

    if (playback->from == Playback::none) {
        return;
    }
    const animation::Clock fading(playback->fromDuration, playback->fromLoops);
    playback->fromTime = fading.sample(fading.advance(playback->fromTime, step));
    playback->fade = playback->fadeDuration > 0.0F ? std::min(1.0F, playback->fade + (step / playback->fadeDuration)) : 1.0F;
    if (playback->fade >= 1.0F) {
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
    const animation::Clock clock(current.duration, current.loops);
    return clock.crossed(unrolled(previous, current), current.time, marker);
}

Playback interpolate(const Playback& from, const Playback& to, float alpha) {
    if (from.clip != to.clip) {
        return to;
    }
    Playback blended = to;
    blended.time = glm::mix(unrolled(from, to), to.time, alpha);
    if (to.from != Playback::none && from.from == to.from) {
        // fromTime is kept wrapped. When it wraps between the steps, draw the later step's
        // time instead of sweeping back through the clip
        blended.fromTime = from.fromTime <= to.fromTime ? glm::mix(from.fromTime, to.fromTime, alpha) : to.fromTime;
        blended.fade = glm::mix(from.fade, to.fade, alpha);
    }
    return blended;
}

};  // namespace v3d::ecs::component

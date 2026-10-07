/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <limits>

#include <entt/entt.hpp>

namespace v3d::ecs::component {

/**
 * Which of a model's clips an entity is playing, how far in, and the one it is fading out of.
 *
 * Advanced on the fixed step by advance(), and drawn between steps through ecs::interpolated,
 * so a game snapshots it beside its Transform. A pose is sampled from it when it is drawn; it
 * holds no pose itself. The game decides which clip plays and when, by calling play().
 *
 * A clip is named by its index into the model's clips, and carries its duration and whether it
 * loops, copied in by play(), so that advancing needs nothing but the component.
 **/
struct Playback final {
    /**
     * The clip of a playback that has been asked to play nothing, which draws the rest pose.
     **/
    static constexpr uint32_t none = std::numeric_limits<uint32_t>::max();

    uint32_t clip{ none };
    float time{ 0.0f };      /**< seconds in, unwrapped when the clip loops - Clock::sample() wraps it **/
    float duration{ 0.0f };
    bool loops{ true };

    uint32_t from{ none };   /**< the clip being faded out of, or none **/
    float fromTime{ 0.0f };
    float fromDuration{ 0.0f };
    bool fromLoops{ true };

    float fade{ 1.0f };      /**< how far the fade into clip has gone, 1 when it has finished or there is none **/
    float fadeDuration{ 0.0f };
};

/**
 * Start a clip from its beginning, fading into it from whatever was playing.
 *
 * Asking for the clip already playing changes nothing, so a game can call this every step from
 * whatever decides its clip. A fade of zero, or nothing playing to fade from, cuts.
 *
 * @param duration the clip's, in seconds
 * @param fade how long the fade lasts, in seconds
 **/
void play(Playback* playback, uint32_t clip, float duration, bool loops, float fade);

/**
 * Move one playback on by a step: its clip, the clip it is fading from, and the fade.
 **/
void advance(Playback* playback, float step);

/**
 * Move every entity's playback on by a step. Called from simulate(), after the snapshot.
 **/
void advance(entt::registry& registry, float step);

/**
 * How many times the step from previous to current passed a marker in the current clip - a
 * footstep, the frame an attack lands. Zero when the step changed clip.
 *
 * @param marker a time within the clip, in seconds
 **/
uint32_t crossed(const Playback& previous, const Playback& current, float marker);

/**
 * The playback alpha of the way from one step to the next, which ecs::interpolated uses to
 * draw it between steps.
 *
 * Across a step that changed clip there is no halfway, so it is the later step, and the fade is
 * what smooths the change. A fade that finished during the step draws the clip alone.
 **/
Playback interpolate(const Playback& from, const Playback& to, float alpha);

};  // namespace v3d::ecs::component

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
 * The animation clip an entity is playing, how far into it, and the clip it is fading out of.
 *
 * advance() moves it forward once per simulation step. To draw between steps, a game keeps a
 * Previous<Playback> next to its Previous<Transform>, and ecs::interpolated() blends the two
 * values. The component holds no pose. The pose is sampled from it at draw time.
 *
 * The game chooses which clip plays, and when, by calling play(). A clip is an index into the
 * model's clips. play() copies the clip's duration and looping flag into the component, so
 * advance() needs no access to the model.
 **/
struct Playback final {
    /**
     * The clip value that means nothing plays. A playback with this clip draws the rest pose.
     **/
    static constexpr uint32_t none = std::numeric_limits<uint32_t>::max();

    uint32_t clip{ none };
    float time{ 0.0F };      /**< seconds into the clip, unwrapped when it loops. Clock::sample() wraps it **/
    float duration{ 0.0F };
    bool loops{ true };

    uint32_t from{ none };   /**< the clip being faded out of, or none **/
    float fromTime{ 0.0F };
    float fromDuration{ 0.0F };
    bool fromLoops{ true };

    float fade{ 1.0F };      /**< how far the fade into clip has gone, 1 when it has finished or there is none **/
    float fadeDuration{ 0.0F };
};

/**
 * The arguments to play(). play() starts a clip from its beginning, and fades into it from the
 * clip that was playing.
 *
 * Playing the clip that is already playing changes nothing, so a game can call play() every
 * step. With a fade of zero, or no clip playing before, the new clip starts at full weight.
 *
 * The clip's duration and the fade are in seconds.
 **/
struct Start {
    uint32_t clip = Playback::none;
    float duration = 0.0F;
    bool loops = false;
    float fade = 0.0F;
};

void play(Playback* playback, const Start& start);

/**
 * Move one playback on by a step: its clip, the clip it is fading from, and the fade.
 **/
void advance(Playback* playback, float step);

/**
 * Move every entity's playback on by a step. Call it from simulate(), after snapshot().
 **/
void advance(entt::registry& registry, float step);

/**
 * How many times the step from previous to current passed marker in the current clip. A marker
 * is a moment such as a footstep, or the frame an attack lands. Zero when the step changed clip
 * or nothing plays.
 *
 * @param marker a time within the clip, in seconds
 **/
uint32_t crossed(const Playback& previous, const Playback& current, float marker);

/**
 * The playback alpha of the way from one step to the next. ecs::interpolated() uses it to draw
 * a playback between steps.
 *
 * When the step changed clip, the result is the later step unchanged, and the fade smooths the
 * change. When a fade finished during the step, the result draws the new clip alone.
 **/
Playback interpolate(const Playback& from, const Playback& to, float alpha);

};  // namespace v3d::ecs::component

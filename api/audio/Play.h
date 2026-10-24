/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

namespace v3d::audio {

/**
 * How to start a sound. Every field defaults to what a one shot needs, so a default Play
 * starts a sound as playClip() does.
 **/
struct Play final {
    Play() noexcept;

    /**
     * The bus the sound is mixed on, such as "music", "sfx" or "ambience". A bus is a named
     * group of sounds whose level Engine::busGain() sets together. Empty is no bus.
     **/
    std::string bus;

    /**
     * How many times to repeat after the first play. -1 loops until stopped, as a bed of
     * ambience or a music track needs.
     **/
    int loops;

    /**
     * How long to fade up from silence, in milliseconds, so a bed starts without a click.
     **/
    int fadeInMs;

    /**
     * The sound's own volume. 1 is unchanged. The sound plays at this times its bus's
     * level.
     **/
    float gain;

    /**
     * Where the sound sits between the speakers: -1 is left, 0 the centre and 1 right. The
     * game converts a position to a pan, because which way is left depends on its camera.
     * stereo() gives the gains a pan becomes.
     **/
    float pan;
};

/**
 * The gain of each side of a panned sound.
 **/
struct Stereo final {
    float left;
    float right;
};

/**
 * The gains a pan gives each side. The side the sound is panned towards stays at full gain. The
 * other falls in a straight line to silence at the end of the range, so a centred sound is
 * exactly an unpanned one. A pan past either end is held there, and one that is not finite is
 * the centre.
 **/
Stereo stereo(float pan) noexcept;

};  // namespace v3d::audio

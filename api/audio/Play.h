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
     * The bus the sound is mixed on - "music", "sfx", "ambience". A bus is a named group
     * with its own volume, so a settings screen can offer a slider per bus. Empty is no
     * bus.
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
     * The sound's own volume, multiplied by its bus's. 1 is unchanged.
     **/
    float gain;
};

};  // namespace v3d::audio

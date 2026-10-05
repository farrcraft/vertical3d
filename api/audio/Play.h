/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

namespace v3d::audio {

/**
 * How to start a sound. Every field has the value a one shot wants, so the default is what
 * playClip() has always done.
 **/
struct Play final {
    Play() noexcept;

    /**
     * The bus the sound is mixed on - "music", "sfx", "ambience". A tag is a named group
     * with a volume for the cost of a string, which is what makes a settings screen three
     * sliders rather than one. Empty is no bus.
     **/
    std::string bus;

    /**
     * How many times to repeat after the first play. -1 loops until stopped, which is what
     * a bed of ambience or a music track wants.
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

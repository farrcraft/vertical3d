/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

namespace v3d::type::animation {

/**
 * The time-keeping of anything played on the step: a duration, and whether it loops or stops
 * at its end - ADR-0070.
 *
 * A clock holds no time of its own. Whoever plays something keeps the time and asks the clock
 * what a step does to it, so a skeletal clip and a sprite's frames can each own theirs. A
 * looping clock's time is kept unwrapped, so that two steps either side of a wrap still
 * interpolate forwards; sample() is what wraps it.
 **/
class Clock final {
 public:
    /**
     * @param duration in seconds. Zero or less is a clock that never moves
     * @param loops whether time wraps at the end, or stops there
     **/
    Clock(float duration, bool loops) noexcept;

    float duration() const noexcept;
    bool loops() const noexcept;

    /**
     * @param time where playback stands, unwrapped
     * @param step seconds of simulated time
     * @return where it stands a step later: unwrapped when looping, and held at the duration
     *         when not
     **/
    float advance(float time, float step) const noexcept;

    /**
     * @return the time within the clip that an unwrapped time stands at, in [0, duration]
     **/
    float sample(float time) const noexcept;

    /**
     * How many times a step passed a marker: a time within the clip at which something is to
     * be reported. A step that ends exactly on the marker passes it, and one that starts on it
     * does not, so a marker is reported once however the steps fall.
     *
     * A marker at the duration is the end: a clamped clip reports it once, the step it stops,
     * and a looping one reports every wrap.
     *
     * @param from where the step started, unwrapped
     * @param to where advance() put it
     **/
    uint32_t crossed(float from, float to, float marker) const noexcept;

    /**
     * @return whether a clamped clip has reached its end. A looping one never has
     **/
    bool finished(float time) const noexcept;

 private:
    float duration_;
    bool loops_;
};

};  // namespace v3d::type::animation

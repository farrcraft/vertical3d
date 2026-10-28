/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

namespace v3d::animation {

/**
 * The timing of anything that advances once per simulation step: a duration, and whether it
 * loops or stops at its end.
 *
 * A clock holds no time of its own. The caller keeps the time and passes it to advance(), so a
 * skeletal clip and a sprite's frames can each keep their own. A looping clock's time is kept
 * unwrapped, so two steps on either side of a wrap still interpolate forwards. sample() wraps
 * it.
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
     * How many times a step passed a marker. A marker is a time within the clip at which
     * something is to be reported. A step that ends exactly on the marker passes it, and one
     * that starts on it does not, so a marker is reported once however the steps fall.
     *
     * A marker at the duration is the end. A clamped clip reports it once, on the step that
     * reaches it, and a looping one reports it at every wrap. A marker outside [0, duration]
     * is never passed.
     *
     * A step that starts or ends at a time that is not finite passes nothing, and neither does
     * a marker that is not a number. A count too large for the result is held at its largest
     * value.
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

};  // namespace v3d::animation

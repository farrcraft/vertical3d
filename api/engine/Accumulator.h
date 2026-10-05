/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

namespace v3d::engine {

/**
 * Real time in, whole simulation steps out.
 *
 * The loop passes each frame's elapsed time to accumulate() and then drains the steps now
 * due, one fixed step at a time. The timing arithmetic lives here, apart from the loop, so it
 * can be tested without a window or a device.
 **/
class Accumulator final {
 public:
    /**
     * The fixed simulation step, in nanoseconds. 60 Hz, and a constant rather than a
     * setting: two machines that disagree about the step disagree about physics.
     **/
    static constexpr std::uint64_t step = 1000000000ULL / 60ULL;

    /**
     * The same step in seconds, as simulate() receives it and as a velocity is
     * expressed against.
     **/
    static constexpr float seconds = 1.0f / 60.0f;

    /**
     * The longest frame real time is taken from. A window drag or a breakpoint produces a
     * delta measured in seconds, and without a ceiling that is hundreds of steps in one
     * frame, each making the next frame later still. The excess is dropped, so the world
     * runs slow for a moment instead of the loop spiralling.
     **/
    static constexpr std::uint64_t clamp = 250000000ULL;

    /**
     * Take one frame of elapsed real time.
     *
     * @param elapsed nanoseconds since the previous frame, clamped before it is added
     * @return how many whole steps are now due, which is how many times drain() will
     *         return true before it stops
     **/
    unsigned int accumulate(std::uint64_t elapsed) noexcept;

    /**
     * Consume one due step.
     *
     * @return whether a step was due, and therefore whether simulate() should run
     **/
    bool drain() noexcept;

    /**
     * The fraction of a step held but not yet drained, in [0, 1).
     *
     * A draw interpolates between the last two simulation states by this much.
     **/
    float alpha() const noexcept;

    /**
     * @return steps made due by the most recent accumulate(), whether or not they were drained
     **/
    unsigned int steps() const noexcept;

    /**
     * @return total time drained as steps, in nanoseconds
     **/
    std::uint64_t simulated() const noexcept;

 private:
     std::uint64_t remainder_ { 0 };
     std::uint64_t simulated_ { 0 };
     unsigned int owed_ { 0 };
     unsigned int steps_ { 0 };
};

};  // namespace v3d::engine

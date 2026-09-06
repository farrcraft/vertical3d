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
 * The loop hands each frame's elapsed time to accumulate() and then drains what that owes,
 * one fixed step at a time. Everything about the timing contract in
 * ADR-0032 that is arithmetic rather than plumbing lives here, which is what makes it
 * testable without a window or a device.
 **/
class Accumulator final {
 public:
    /**
     * The fixed simulation step, in nanoseconds. 60 Hz, and a constant rather than a
     * setting: two machines that disagree about the step disagree about physics.
     **/
    static constexpr std::uint64_t step = 1000000000ULL / 60ULL;

    /**
     * The same step in seconds, which is what simulate() is passed and what a velocity is
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
     * @return how many whole steps are now owed, which is how many times drain() will
     *         return true before it stops
     **/
    unsigned int accumulate(std::uint64_t elapsed) noexcept;

    /**
     * Consume one owed step.
     *
     * @return whether a step was owed, and therefore whether simulate() should run
     **/
    bool drain() noexcept;

    /**
     * The fraction of a step held but not yet drained, in [0, 1).
     *
     * Rendering between the last completed step and the one after it is the entire reason
     * to separate simulation from drawing. Nothing reads this yet; it is here so that the
     * app which wants it does not have to rewrite every draw call in the tree first.
     **/
    float alpha() const noexcept;

    /**
     * @return steps owed by the most recent accumulate(), whether or not they were drained
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

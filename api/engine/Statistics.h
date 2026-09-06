/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>
#include <cstdint>

namespace v3d::engine {

/**
 * What the loop knows about its own pacing.
 *
 * Collection belongs here because the loop is the only thing that measures a frame;
 * drawing any of it stays with whoever wants to draw it.
 **/
class Statistics final {
 public:
    /**
     * How many frames the rolling mean covers. Half a second at 120 Hz, which is long
     * enough to be steady and short enough to react.
     **/
    static constexpr std::size_t window = 64;

    /**
     * Record one frame.
     *
     * @param frame nanoseconds the frame took, before the accumulator clamps it
     * @param steps simulation steps that frame owed
     **/
    void frame(std::uint64_t frame, unsigned int steps) noexcept;

    /**
     * @return the last frame, in nanoseconds
     **/
    std::uint64_t last() const noexcept;

    /**
     * @return the mean frame over the window, in nanoseconds, or zero before the first
     **/
    std::uint64_t mean() const noexcept;

    /**
     * Steps the last frame owed.
     *
     * This is the number worth watching. It sits at 0 or 1 with occasional 2s on a healthy
     * frame; a sustained 3 or more means the accumulator's clamp is doing real work and
     * something cannot keep up with the step it is being given.
     **/
    unsigned int steps() const noexcept;

    /**
     * @return frames recorded since the loop started
     **/
    std::uint64_t frames() const noexcept;

 private:
     std::array<std::uint64_t, window> recent_ {};
     std::uint64_t total_ { 0 };
     std::uint64_t frames_ { 0 };
     std::uint64_t last_ { 0 };
     std::size_t next_ { 0 };
     unsigned int steps_ { 0 };
};

};  // namespace v3d::engine

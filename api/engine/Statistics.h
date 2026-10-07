/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace v3d::engine {

/**
 * What the loop measures about its own pacing.
 *
 * The loop is the only thing that measures a frame, so collection lives here. Displaying
 * the numbers is up to the app.
 **/
class Statistics final {
 public:
    /**
     * Where a scope reads the time, in nanoseconds. A steady clock unless a test hands in
     * one it can step.
     **/
    typedef std::function<std::uint64_t()> Clock;

    /**
     * Time spent under a name, as of the last frame and over the window.
     **/
    struct Row final {
        std::string name;
        std::uint64_t last { 0 };  /**< nanoseconds in the last frame **/
        std::uint64_t mean { 0 };  /**< nanoseconds a frame, over the window **/
    };

    /**
     * A span of the frame being timed, which adds its lifetime to its name's row when it
     * ends. Held for the length of whatever is being timed, and not kept past the frame.
     **/
    class Scope final {
     public:
        ~Scope();
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        Scope(Scope&& other) noexcept;
        Scope& operator=(Scope&&) = delete;

     private:
        friend class Statistics;
        Scope(Statistics* statistics, std::size_t row);

        Statistics* statistics_;
        std::size_t row_;
        std::uint64_t start_;
    };

    Statistics();

    /**
     * How many frames the rolling mean covers. Half a second at 120 Hz, which is long
     * enough to be steady and short enough to react.
     **/
    static constexpr std::size_t window = 64;

    /**
     * Record one frame.
     *
     * @param frame nanoseconds the frame took, before the accumulator clamps it
     * @param steps simulation steps due in that frame
     **/
    void frame(std::uint64_t frame, unsigned int steps) noexcept;

    /**
     * Time what happens until the scope ends, under a name. Two scopes of one name in a frame
     * add up.
     **/
    Scope scope(std::string_view name);

    /**
     * Add time to a name's row for the frame. A scope calls this as it ends, and it is how
     * a time measured elsewhere joins the rows.
     **/
    void add(std::string_view name, std::uint64_t nanoseconds);

    /**
     * @return a row per name timed so far, in the order each was first timed
     **/
    std::vector<Row> rows() const;

    /**
     * Read the time from somewhere else, which is for a test.
     **/
    void clock(const Clock& clock);

    /**
     * @return the last frame, in nanoseconds
     **/
    std::uint64_t last() const noexcept;

    /**
     * @return the mean frame over the window, in nanoseconds, or zero before the first
     **/
    std::uint64_t mean() const noexcept;

    /**
     * Steps due in the last frame.
     *
     * This is the number worth watching. It sits at 0 or 1 with occasional 2s on a healthy
     * frame; a sustained 3 or more means frames take longer than the fixed step and the loop
     * is falling behind.
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

     struct Named final {
         std::string name;
         std::array<std::uint64_t, window> recent {};
         std::uint64_t total { 0 };
         std::uint64_t pending { 0 };
         std::uint64_t last { 0 };
     };
     /**
      * The row a name is timed into, made the first time it is asked for. A scope finds
      * its row as it opens, so that closing one cannot allocate.
      **/
     std::size_t row(std::string_view name);

     std::vector<Named> named_;
     Clock clock_;
};

};  // namespace v3d::engine

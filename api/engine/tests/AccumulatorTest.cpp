/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "../Accumulator.h"

namespace {

using v3d::engine::Accumulator;

/**
 * Run a scripted sequence of frames and report what the accumulator did with them.
 **/
struct Result {
    unsigned int steps { 0 };
    std::uint64_t simulated { 0 };
    unsigned int worst { 0 };
};

Result run(Accumulator* accumulator, const std::vector<std::uint64_t>& frames) {
    Result result;
    for (std::uint64_t frame : frames) {
        unsigned int owed = accumulator->accumulate(frame);
        result.worst = (owed > result.worst) ? owed : result.worst;
        while (accumulator->drain()) {
            result.steps++;
        }
    }
    result.simulated = accumulator->simulated();
    return result;
}

}  // namespace

/**
 * A frame shorter than a step owes nothing, but the time is kept rather than discarded -
 * which is the whole difference from a millisecond delta that rounds to zero.
 **/
BOOST_AUTO_TEST_CASE(accumulator_short_frames_test) {
    Accumulator accumulator;
    BOOST_CHECK_EQUAL(accumulator.accumulate(1000), 0u);
    BOOST_CHECK_EQUAL(accumulator.steps(), 0u);
    BOOST_CHECK(!accumulator.drain());

    // 16 ms of microsecond frames is still short of a step, and still owes nothing - a
    // millisecond clock would have reported zero for every one of them and lost all 16
    const std::vector<std::uint64_t> frames(15999, 1000);
    Result result = run(&accumulator, frames);
    BOOST_CHECK_EQUAL(result.steps, 0u);

    // another millisecond of them takes it over
    const std::vector<std::uint64_t> rest(1000, 1000);
    result = run(&accumulator, rest);
    BOOST_CHECK_EQUAL(result.steps, 1u);
}

/**
 * A frame of exactly one step owes exactly one, and leaves nothing behind.
 **/
BOOST_AUTO_TEST_CASE(accumulator_whole_step_test) {
    Accumulator accumulator;
    BOOST_CHECK_EQUAL(accumulator.accumulate(Accumulator::step), 1u);
    BOOST_CHECK(accumulator.drain());
    BOOST_CHECK(!accumulator.drain());
    BOOST_CHECK_EQUAL(accumulator.simulated(), Accumulator::step);
    BOOST_CHECK_SMALL(accumulator.alpha(), 0.0001f);
}

/**
 * 144 Hz alternating 6 and 7 ms is what the millisecond clock produced, and it is the case
 * a fixed step exists to make well behaved: over a second the frames still add up to a
 * second of simulation, whatever any individual frame owed.
 **/
BOOST_AUTO_TEST_CASE(accumulator_jitter_test) {
    Accumulator accumulator;
    std::vector<std::uint64_t> frames;
    frames.reserve(144);
    for (int i = 0; i < 144; ++i) {
        frames.push_back((i % 2 == 0) ? 6000000ULL : 7000000ULL);
    }
    Result result = run(&accumulator, frames);

    // 72 frames of 6 ms and 72 of 7 ms is 936 ms, which is 56 whole steps of 16.667 ms
    BOOST_CHECK_EQUAL(result.steps, 56u);
    BOOST_CHECK_EQUAL(result.simulated, 56ULL * Accumulator::step);
    // no frame is longer than a step, so no frame ever owes more than one
    BOOST_CHECK_EQUAL(result.worst, 1u);
}

/**
 * The same simulated duration however it was paced. This is the property the whole record
 * is for: a scene stepped 600 times is a scene stepped 600 times.
 **/
BOOST_AUTO_TEST_CASE(accumulator_pacing_test) {
    const std::uint64_t second = 1000000000ULL;

    Accumulator smooth;
    std::vector<std::uint64_t> even(60, second / 60);
    Result a = run(&smooth, even);

    Accumulator ragged;
    std::vector<std::uint64_t> uneven;
    for (int i = 0; i < 30; ++i) {
        uneven.push_back(second / 120);
        uneven.push_back(second / 40);
    }
    Result b = run(&ragged, uneven);

    BOOST_CHECK_EQUAL(a.steps, b.steps);
    BOOST_CHECK_EQUAL(a.simulated, b.simulated);
}

/**
 * A window drag or a breakpoint hands the loop seconds. Without the clamp that is hundreds
 * of steps in one frame, each making the next frame later still; with it the world runs
 * slow for a moment, which is the correct failure.
 **/
BOOST_AUTO_TEST_CASE(accumulator_clamp_test) {
    Accumulator accumulator;
    unsigned int owed = accumulator.accumulate(30000000000ULL);
    BOOST_CHECK_EQUAL(owed, static_cast<unsigned int>(Accumulator::clamp / Accumulator::step));
    BOOST_CHECK_EQUAL(owed, 15u);

    unsigned int drained = 0;
    while (accumulator.drain()) {
        drained++;
    }
    BOOST_CHECK_EQUAL(drained, 15u);
    // thirty seconds arrived and a quarter of a second of it was simulated - the rest is gone
    BOOST_CHECK_EQUAL(accumulator.simulated(), 15ULL * Accumulator::step);
}

/**
 * The remainder read as a fraction, which is what a renderer interpolating between two
 * simulation states blends by.
 **/
BOOST_AUTO_TEST_CASE(accumulator_alpha_test) {
    Accumulator accumulator;
    accumulator.accumulate(Accumulator::step / 2);
    BOOST_CHECK_CLOSE(accumulator.alpha(), 0.5f, 0.1f);

    // a whole step plus a half drains one and leaves the half
    accumulator.accumulate(Accumulator::step);
    BOOST_CHECK(accumulator.drain());
    BOOST_CHECK(!accumulator.drain());
    BOOST_CHECK_CLOSE(accumulator.alpha(), 0.5f, 0.1f);
}

/**
 * steps() reports what the last accumulate() found owed, which is the number worth watching:
 * 0 or 1 with occasional 2s is healthy, and a sustained 3 means the clamp is doing real work.
 **/
BOOST_AUTO_TEST_CASE(accumulator_steps_test) {
    Accumulator accumulator;
    accumulator.accumulate(Accumulator::step * 3);
    BOOST_CHECK_EQUAL(accumulator.steps(), 3u);
    accumulator.accumulate(0);
    BOOST_CHECK_EQUAL(accumulator.steps(), 3u);
    while (accumulator.drain()) {
    }
    accumulator.accumulate(0);
    BOOST_CHECK_EQUAL(accumulator.steps(), 0u);
}

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/Accumulator.h>

#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

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
        const unsigned int owed = accumulator->accumulate(frame);
        result.worst = (owed > result.worst) ? owed : result.worst;
        while (accumulator->drain()) {
            result.steps++;
        }
    }
    result.simulated = accumulator->simulated();
    return result;
}

}  // namespace

namespace {

/**
 * A frame shorter than a step makes no step due, but its time is kept rather than discarded,
 * unlike a millisecond delta that rounds to zero.
 **/
BOOST_AUTO_TEST_CASE(accumulator_short_frames_test) {
    Accumulator accumulator;
    BOOST_CHECK_EQUAL(accumulator.accumulate(1000), 0U);
    BOOST_CHECK_EQUAL(accumulator.steps(), 0U);
    BOOST_CHECK(!accumulator.drain());

    // 16 ms of microsecond frames is still short of a step; a millisecond clock would
    // report zero for each of them and lose all 16
    const std::vector<std::uint64_t> frames(15999, 1000);
    Result result = run(&accumulator, frames);
    BOOST_CHECK_EQUAL(result.steps, 0U);

    // another millisecond of them takes it over
    const std::vector<std::uint64_t> rest(1000, 1000);
    result = run(&accumulator, rest);
    BOOST_CHECK_EQUAL(result.steps, 1U);
}

/**
 * A frame of exactly one step makes exactly one due, and leaves nothing behind.
 **/
BOOST_AUTO_TEST_CASE(accumulator_whole_step_test) {
    Accumulator accumulator;
    BOOST_CHECK_EQUAL(accumulator.accumulate(Accumulator::step), 1U);
    BOOST_CHECK(accumulator.drain());
    BOOST_CHECK(!accumulator.drain());
    BOOST_CHECK_EQUAL(accumulator.simulated(), Accumulator::step);
    BOOST_CHECK_SMALL(accumulator.alpha(), 0.0001F);
}

/**
 * 144 Hz measured in whole milliseconds alternates 6 and 7 ms frames. Over a second they
 * still add up to a second of simulation, whatever any one frame made due.
 **/
BOOST_AUTO_TEST_CASE(accumulator_jitter_test) {
    Accumulator accumulator;
    std::vector<std::uint64_t> frames;
    frames.reserve(144);
    for (int i = 0; i < 144; ++i) {
        frames.push_back((i % 2 == 0) ? 6000000ULL : 7000000ULL);
    }
    const Result result = run(&accumulator, frames);

    // 72 frames of 6 ms and 72 of 7 ms is 936 ms, which is 56 whole steps of 16.667 ms
    BOOST_CHECK_EQUAL(result.steps, 56U);
    BOOST_CHECK_EQUAL(result.simulated, 56ULL * Accumulator::step);
    // no frame is longer than a step, so no frame ever makes more than one due
    BOOST_CHECK_EQUAL(result.worst, 1U);
}

/**
 * The same real duration gives the same number of steps however its frames were paced.
 **/
BOOST_AUTO_TEST_CASE(accumulator_pacing_test) {
    const std::uint64_t second = 1000000000ULL;

    Accumulator smooth;
    const std::vector<std::uint64_t> even(60, second / 60);
    const Result smoothResult = run(&smooth, even);

    Accumulator ragged;
    std::vector<std::uint64_t> uneven;
    for (int i = 0; i < 30; ++i) {
        uneven.push_back(second / 120);
        uneven.push_back(second / 40);
    }
    const Result raggedResult = run(&ragged, uneven);

    BOOST_CHECK_EQUAL(smoothResult.steps, raggedResult.steps);
    BOOST_CHECK_EQUAL(smoothResult.simulated, raggedResult.simulated);
}

/**
 * A window drag or a breakpoint gives the loop a frame several seconds long. The clamp
 * limits it, so the world runs slow for a moment instead of running hundreds of steps in one
 * frame.
 **/
BOOST_AUTO_TEST_CASE(accumulator_clamp_test) {
    Accumulator accumulator;
    const unsigned int owed = accumulator.accumulate(30000000000ULL);
    BOOST_CHECK_EQUAL(owed, static_cast<unsigned int>(Accumulator::clamp / Accumulator::step));
    BOOST_CHECK_EQUAL(owed, 15U);

    unsigned int drained = 0;
    while (accumulator.drain()) {
        drained++;
    }
    BOOST_CHECK_EQUAL(drained, 15U);
    // thirty seconds arrived and a quarter of a second of it was simulated - the rest is gone
    BOOST_CHECK_EQUAL(accumulator.simulated(), 15ULL * Accumulator::step);
}

/**
 * The remainder read as a fraction, which a renderer interpolating between two simulation
 * states blends by.
 **/
BOOST_AUTO_TEST_CASE(accumulator_alpha_test) {
    Accumulator accumulator;
    accumulator.accumulate(Accumulator::step / 2);
    BOOST_CHECK_CLOSE(accumulator.alpha(), 0.5F, 0.1F);

    // a whole step plus a half drains one and leaves the half
    accumulator.accumulate(Accumulator::step);
    BOOST_CHECK(accumulator.drain());
    BOOST_CHECK(!accumulator.drain());
    BOOST_CHECK_CLOSE(accumulator.alpha(), 0.5F, 0.1F);
}

/**
 * steps() reports how many steps the last accumulate() made due.
 **/
BOOST_AUTO_TEST_CASE(accumulator_steps_test) {
    Accumulator accumulator;
    accumulator.accumulate(Accumulator::step * 3);
    BOOST_CHECK_EQUAL(accumulator.steps(), 3U);
    accumulator.accumulate(0);
    BOOST_CHECK_EQUAL(accumulator.steps(), 3U);
    while (accumulator.drain()) {
    }
    accumulator.accumulate(0);
    BOOST_CHECK_EQUAL(accumulator.steps(), 0U);
}

};  // namespace

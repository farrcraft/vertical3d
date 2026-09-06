/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <cstdint>

#include <boost/test/unit_test.hpp>

#include "../Accumulator.h"
#include "../Statistics.h"

namespace {

using v3d::engine::Accumulator;
using v3d::engine::Statistics;

constexpr std::uint64_t MS = 1000000ULL;

};  // namespace

/**
 * Nothing has happened yet, so there is no mean to report rather than a mean of an array
 * of zeros.
 **/
BOOST_AUTO_TEST_CASE(statistics_empty_test) {
    Statistics statistics;

    BOOST_CHECK_EQUAL(statistics.frames(), 0u);
    BOOST_CHECK_EQUAL(statistics.last(), 0u);
    BOOST_CHECK_EQUAL(statistics.mean(), 0u);
    BOOST_CHECK_EQUAL(statistics.steps(), 0u);
}

/**
 * Before the window has filled the mean covers what there is, so the first frames are not
 * averaged against slots that never held a frame.
 **/
BOOST_AUTO_TEST_CASE(statistics_partial_window_test) {
    Statistics statistics;

    statistics.frame(10 * MS, 1);
    BOOST_CHECK_EQUAL(statistics.last(), 10 * MS);
    BOOST_CHECK_EQUAL(statistics.mean(), 10 * MS);

    statistics.frame(20 * MS, 1);
    BOOST_CHECK_EQUAL(statistics.mean(), 15 * MS);
    BOOST_CHECK_EQUAL(statistics.frames(), 2u);
}

/**
 * The window is rolling: a frame that leaves it stops counting, so a stall is forgotten
 * rather than held against the mean forever.
 **/
BOOST_AUTO_TEST_CASE(statistics_window_rolls_test) {
    Statistics statistics;

    statistics.frame(1000 * MS, 15);
    for (std::size_t i = 0; i < Statistics::window; ++i) {
        statistics.frame(10 * MS, 1);
    }

    BOOST_CHECK_EQUAL(statistics.mean(), 10 * MS);
    BOOST_CHECK_EQUAL(statistics.frames(), Statistics::window + 1);
}

/**
 * Steps-per-frame is the health metric, and it reports what the frame owed rather than
 * what the clamp let through - a frame owing 15 is the clamp at its ceiling.
 **/
BOOST_AUTO_TEST_CASE(statistics_steps_test) {
    Accumulator accumulator;
    Statistics statistics;

    statistics.frame(Accumulator::step, accumulator.accumulate(Accumulator::step));
    BOOST_CHECK_EQUAL(statistics.steps(), 1u);
    while (accumulator.drain()) {
    }

    const std::uint64_t stall = 30000 * MS;
    statistics.frame(stall, accumulator.accumulate(stall));
    BOOST_CHECK_EQUAL(statistics.steps(), 15u);
    // the frame is recorded as it was measured, not as the accumulator clamped it
    BOOST_CHECK_EQUAL(statistics.last(), stall);
}

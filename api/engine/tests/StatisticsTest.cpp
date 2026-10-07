/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/Accumulator.h>
#include <api/engine/Statistics.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

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
 * Steps per frame is the health metric. A stalled frame reports 15 steps, the clamp's
 * ceiling, while its frame time is recorded unclamped.
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

/**
 * A scope adds the time it was open to its name's row, read off whatever clock the statistics
 * were handed, and the row reports it once the frame closes.
 **/
BOOST_AUTO_TEST_CASE(statistics_scope_test) {
    Statistics statistics;
    std::uint64_t now = 0;
    statistics.clock([&now]() { return now; });

    {
        const Statistics::Scope timed = statistics.scope("chunks");
        now += 3 * MS;
    }
    statistics.frame(16 * MS, 1);

    const std::vector<Statistics::Row> rows = statistics.rows();
    BOOST_REQUIRE_EQUAL(rows.size(), 1u);
    BOOST_CHECK_EQUAL(rows[0].name, "chunks");
    BOOST_CHECK_EQUAL(rows[0].last, 3 * MS);
    BOOST_CHECK_EQUAL(rows[0].mean, 3 * MS);
}

/**
 * Two scopes of one name in a frame add up, and names keep the order they were first timed in.
 **/
BOOST_AUTO_TEST_CASE(statistics_scopes_of_one_name_add_up_test) {
    Statistics statistics;
    std::uint64_t now = 0;
    statistics.clock([&now]() { return now; });

    {
        const Statistics::Scope first = statistics.scope("physics");
        now += 1 * MS;
    }
    {
        const Statistics::Scope other = statistics.scope("audio");
        now += 5 * MS;
    }
    {
        const Statistics::Scope second = statistics.scope("physics");
        now += 2 * MS;
    }
    statistics.frame(16 * MS, 1);

    const std::vector<Statistics::Row> rows = statistics.rows();
    BOOST_REQUIRE_EQUAL(rows.size(), 2u);
    BOOST_CHECK_EQUAL(rows[0].name, "physics");
    BOOST_CHECK_EQUAL(rows[0].last, 3 * MS);
    BOOST_CHECK_EQUAL(rows[1].name, "audio");
    BOOST_CHECK_EQUAL(rows[1].last, 5 * MS);
}

/**
 * A name's mean rolls over the same window as the frame's, and a frame it was not timed in
 * counts as one it took no time in.
 **/
BOOST_AUTO_TEST_CASE(statistics_row_mean_rolls_test) {
    Statistics statistics;
    statistics.add("draw", 4 * MS);
    statistics.frame(16 * MS, 1);
    statistics.frame(16 * MS, 1);

    const std::vector<Statistics::Row> rows = statistics.rows();
    BOOST_REQUIRE_EQUAL(rows.size(), 1u);
    BOOST_CHECK_EQUAL(rows[0].last, 0u);
    BOOST_CHECK_EQUAL(rows[0].mean, 2 * MS);

    for (std::size_t frame = 0; frame < Statistics::window; frame++) {
        statistics.add("draw", 6 * MS);
        statistics.frame(16 * MS, 1);
    }
    BOOST_CHECK_EQUAL(statistics.rows()[0].mean, 6 * MS);
}

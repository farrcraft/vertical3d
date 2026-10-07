/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/vulkan/frame/Retirement.h>

#include <vector>

#include <boost/test/unit_test.hpp>

using v3d::render::realtime::vulkan::frame::Retirement;

BOOST_AUTO_TEST_SUITE(retirement_test)

/**
 * Something released while frame n was being recorded is still in use until frame n has
 * finished, and with two frames in flight that is known only once frame n + 2 has begun.
 **/
BOOST_AUTO_TEST_CASE(a_destruction_waits_for_the_frames_in_flight) {
    Retirement retirement(2);
    bool destroyed = false;

    retirement.retire(5, [&destroyed]() { destroyed = true; });

    retirement.collect(6);
    BOOST_CHECK(!destroyed);

    retirement.collect(7);
    BOOST_CHECK(destroyed);
    BOOST_CHECK_EQUAL(retirement.pending(), 0u);
}

/**
 * Collecting runs only what is due and leaves the rest, in the order it was retired.
 **/
BOOST_AUTO_TEST_CASE(only_what_is_due_is_collected) {
    Retirement retirement(2);
    std::vector<int> destroyed;

    retirement.retire(1, [&destroyed]() { destroyed.push_back(1); });
    retirement.retire(1, [&destroyed]() { destroyed.push_back(2); });
    retirement.retire(2, [&destroyed]() { destroyed.push_back(3); });

    retirement.collect(3);

    BOOST_REQUIRE_EQUAL(destroyed.size(), 2u);
    BOOST_CHECK_EQUAL(destroyed[0], 1);
    BOOST_CHECK_EQUAL(destroyed[1], 2);
    BOOST_CHECK_EQUAL(retirement.pending(), 1u);
}

/**
 * Whatever is still held when the queue goes is destroyed rather than leaked - the ring idles
 * the device before that happens.
 **/
BOOST_AUTO_TEST_CASE(what_is_held_is_destroyed_with_the_queue) {
    bool destroyed = false;
    {
        Retirement retirement(2);
        retirement.retire(0, [&destroyed]() { destroyed = true; });
    }

    BOOST_CHECK(destroyed);
}

BOOST_AUTO_TEST_SUITE_END()

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../3dtypes.h"

/**
 * The angle and float comparison helpers this test was originally written against -
 * rad2deg, deg2rad, float_eq and swap - are gone; glm and <algorithm> cover them. What is
 * left in the header is the pair of bit twiddling helpers the font atlas uses.
 **/
BOOST_AUTO_TEST_CASE(types_test) {
    // floor_log2 is the index of the highest set bit
    BOOST_CHECK_EQUAL(floor_log2(1), 0);
    BOOST_CHECK_EQUAL(floor_log2(2), 1);
    BOOST_CHECK_EQUAL(floor_log2(3), 1);
    BOOST_CHECK_EQUAL(floor_log2(255), 7);
    BOOST_CHECK_EQUAL(floor_log2(256), 8);
    BOOST_CHECK_EQUAL(floor_log2(0x80000000), 31);

    // no bit set at all
    BOOST_CHECK_EQUAL(floor_log2(0), -1);

    // npot rounds up to the next power of two, and leaves an exact power of two alone
    BOOST_CHECK_EQUAL(npot(1), 1u);
    BOOST_CHECK_EQUAL(npot(2), 2u);
    BOOST_CHECK_EQUAL(npot(3), 4u);
    BOOST_CHECK_EQUAL(npot(5), 8u);
    BOOST_CHECK_EQUAL(npot(64), 64u);
    BOOST_CHECK_EQUAL(npot(65), 128u);
    BOOST_CHECK_EQUAL(npot(1000), 1024u);
}

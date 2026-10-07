/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <voxel/src/engine/Nanoseconds.h>

#include <cmath>
#include <cstdint>
#include <limits>

#include <boost/test/unit_test.hpp>

/**
 * A span in milliseconds becomes whole nanoseconds, with the fraction dropped.
 **/
BOOST_AUTO_TEST_CASE(nanoseconds_of_a_span_test) {
    BOOST_CHECK_EQUAL(nanoseconds(0.0), 0u);
    BOOST_CHECK_EQUAL(nanoseconds(1.5), 1500000u);
    BOOST_CHECK_EQUAL(nanoseconds(0.0000015), 1u);
    // 1.8e19 nanoseconds is below 2^64 and exact as a double
    BOOST_CHECK_EQUAL(nanoseconds(1.8e13), 18000000000000000000u);
}

/**
 * A negative span, -infinity and a NaN are zero.
 **/
BOOST_AUTO_TEST_CASE(nanoseconds_of_no_span_test) {
    BOOST_CHECK_EQUAL(nanoseconds(-1.0), 0u);
    BOOST_CHECK_EQUAL(nanoseconds(-std::numeric_limits<double>::infinity()), 0u);
    BOOST_CHECK_EQUAL(nanoseconds(std::nan("")), 0u);
}

/**
 * A span of 2^64 nanoseconds or longer, and +infinity, is the largest count.
 **/
BOOST_AUTO_TEST_CASE(nanoseconds_of_a_span_too_long_test) {
    const std::uint64_t largest = std::numeric_limits<std::uint64_t>::max();
    // just above 2^64 nanoseconds, since 2^64 divided by 1e6 is not exact as a double
    const double twoTo64 = std::ldexp(1.0, 64);
    BOOST_CHECK_EQUAL(nanoseconds(twoTo64 / 1.0e6 * (1.0 + std::ldexp(1.0, -40))), largest);
    BOOST_CHECK_EQUAL(nanoseconds(2.0e13), largest);
    BOOST_CHECK_EQUAL(nanoseconds(1.0e300), largest);
    BOOST_CHECK_EQUAL(nanoseconds(std::numeric_limits<double>::infinity()), largest);
}

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/Checked.h>

#include <cstdint>
#include <limits>

#include <boost/test/unit_test.hpp>

using v3d::type::toCount;
using v3d::type::toInteger;

namespace {

const float notANumber = std::numeric_limits<float>::quiet_NaN();
const float infinity = std::numeric_limits<float>::infinity();

};  // namespace

BOOST_AUTO_TEST_SUITE(checked_test)

BOOST_AUTO_TEST_CASE(a_count_within_its_range_is_truncated) {
    BOOST_CHECK(toCount(0.0f, 10) == 0u);
    BOOST_CHECK(toCount(-0.0f, 10) == 0u);
    BOOST_CHECK(toCount(7.0f, 10) == 7u);
    BOOST_CHECK(toCount(7.9f, 10) == 7u);
    BOOST_CHECK(toCount(10.0f, 10) == 10u);
    BOOST_CHECK(toCount(1.0f, 1, 10) == 1u);
}

BOOST_AUTO_TEST_CASE(a_count_that_is_not_finite_is_refused) {
    BOOST_CHECK(!toCount(notANumber, 10).has_value());
    BOOST_CHECK(!toCount(infinity, 10).has_value());
    BOOST_CHECK(!toCount(-infinity, 10).has_value());
    BOOST_CHECK(!toCount(infinity, std::numeric_limits<uint32_t>::max()).has_value());
}

BOOST_AUTO_TEST_CASE(a_count_outside_its_range_is_refused_before_truncation) {
    BOOST_CHECK(!toCount(-1.0f, 10).has_value());
    // truncation would make these 0 and 10, which are in range
    BOOST_CHECK(!toCount(-0.5f, 10).has_value());
    BOOST_CHECK(!toCount(10.5f, 10).has_value());
    BOOST_CHECK(!toCount(0.5f, 1, 10).has_value());
    BOOST_CHECK(!toCount(5.0f, 6, 4).has_value());
}

BOOST_AUTO_TEST_CASE(a_count_up_to_the_largest_unsigned_is_defined) {
    const uint32_t most = std::numeric_limits<uint32_t>::max();
    // the largest float below 2^32 fits, and 2^32 itself does not, although the float nearest
    // the bound is 2^32
    BOOST_CHECK(toCount(4294967040.0f, most) == 4294967040u);
    BOOST_CHECK(!toCount(4294967296.0f, most).has_value());
}

BOOST_AUTO_TEST_CASE(an_integer_within_its_range_is_truncated_toward_zero) {
    BOOST_CHECK(toInteger(3.7f, -10, 10) == 3);
    BOOST_CHECK(toInteger(-2.5f, -10, 10) == -2);
    BOOST_CHECK(toInteger(-10.0f, -10, 10) == -10);
    BOOST_CHECK(toInteger(10.0f, -10, 10) == 10);
}

BOOST_AUTO_TEST_CASE(an_integer_that_is_not_finite_or_out_of_range_is_refused) {
    BOOST_CHECK(!toInteger(notANumber, -10, 10).has_value());
    BOOST_CHECK(!toInteger(infinity, -10, 10).has_value());
    BOOST_CHECK(!toInteger(-infinity, -10, 10).has_value());
    BOOST_CHECK(!toInteger(10.5f, -10, 10).has_value());
    BOOST_CHECK(!toInteger(-10.5f, -10, 10).has_value());
    BOOST_CHECK(!toInteger(-0.5f, 0, 10).has_value());
}

BOOST_AUTO_TEST_CASE(an_integer_up_to_the_largest_int_is_defined) {
    const int32_t most = std::numeric_limits<int32_t>::max();
    const int32_t least = std::numeric_limits<int32_t>::min();
    BOOST_CHECK(toInteger(2147483520.0f, least, most) == 2147483520);
    BOOST_CHECK(!toInteger(2147483648.0f, least, most).has_value());
    BOOST_CHECK(toInteger(-2147483648.0f, least, most) == least);
    BOOST_CHECK(!toInteger(-2147483904.0f, least, most).has_value());
}

BOOST_AUTO_TEST_SUITE_END()

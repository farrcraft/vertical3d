/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Master Test Suite
#include <boost/test/unit_test.hpp>

/**
 * Global Test Fixture used for initializing the logger
 */
struct LogInitializerFixture {
    LogInitializerFixture() {
    }
};

BOOST_GLOBAL_FIXTURE(LogInitializerFixture)

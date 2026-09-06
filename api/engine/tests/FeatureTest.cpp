/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../Feature.h"

/**
 * The bitmask an app hands initialize(). Combining is done over the enum and testing over the
 * int it arrives as, which is why there are two operators rather than one.
 **/
BOOST_AUTO_TEST_CASE(feature_combine_test) {
    const v3d::engine::Feature features =
        v3d::engine::Feature::Window | v3d::engine::Feature::Config;

    BOOST_CHECK_EQUAL(static_cast<int>(features), 3);
}

BOOST_AUTO_TEST_CASE(feature_test_test) {
    const int features = static_cast<int>(
        v3d::engine::Feature::Config | v3d::engine::Feature::KeyboardInput);

    BOOST_CHECK(features & v3d::engine::Feature::Config);
    BOOST_CHECK(features & v3d::engine::Feature::KeyboardInput);
    BOOST_CHECK(!(features & v3d::engine::Feature::Window));
    BOOST_CHECK(!(features & v3d::engine::Feature::MouseInput));
}

/**
 * Each flag is a bit of its own, so no feature can be asked for by asking for another.
 **/
BOOST_AUTO_TEST_CASE(feature_flags_are_distinct_test) {
    BOOST_CHECK_EQUAL(static_cast<int>(v3d::engine::Feature::Window), 1);
    BOOST_CHECK_EQUAL(static_cast<int>(v3d::engine::Feature::Config), 2);
    BOOST_CHECK_EQUAL(static_cast<int>(v3d::engine::Feature::MouseInput), 4);
    BOOST_CHECK_EQUAL(static_cast<int>(v3d::engine::Feature::KeyboardInput), 8);
}

/**
 * Nothing asked for is nothing enabled, which is the mask a test - or an app that only wants
 * the asset manager and the event engine - passes.
 **/
BOOST_AUTO_TEST_CASE(feature_none_test) {
    const int features = 0;

    BOOST_CHECK(!(features & v3d::engine::Feature::Window));
    BOOST_CHECK(!(features & v3d::engine::Feature::Config));
    BOOST_CHECK(!(features & v3d::engine::Feature::MouseInput));
    BOOST_CHECK(!(features & v3d::engine::Feature::KeyboardInput));
}

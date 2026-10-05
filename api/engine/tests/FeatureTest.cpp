/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/Feature.h>
#include <api/input/DeviceType.h>

#include <boost/test/unit_test.hpp>

/**
 * What an app asks the engine for. Combining features is a set of them, and a set answers
 * only for the enum it holds.
 **/
BOOST_AUTO_TEST_CASE(feature_combine_test) {
    const v3d::engine::Features features = v3d::engine::Feature::Config | v3d::engine::Feature::KeyboardInput;

    BOOST_CHECK(features.has(v3d::engine::Feature::Config));
    BOOST_CHECK(features.has(v3d::engine::Feature::KeyboardInput));
    BOOST_CHECK(!features.has(v3d::engine::Feature::Window));
    BOOST_CHECK(!features.has(v3d::engine::Feature::MouseInput));
}

/**
 * Nothing asked for is nothing enabled, which is the set a test passes when it wants only the
 * asset manager and the event engine.
 **/
BOOST_AUTO_TEST_CASE(feature_none_test) {
    const v3d::engine::Features features;

    BOOST_CHECK(features.empty());
    BOOST_CHECK(!features.has(v3d::engine::Feature::Window));
}

/**
 * A set grows by |=, and one bit is a set of one wherever a set is asked for - which is how
 * the engine builds the devices it hands the input engine.
 **/
BOOST_AUTO_TEST_CASE(feature_accumulate_test) {
    v3d::input::DeviceTypes devices;
    devices |= v3d::input::DeviceType::Mouse;

    BOOST_CHECK(devices.has(v3d::input::DeviceType::Mouse));
    BOOST_CHECK(!devices.has(v3d::input::DeviceType::Keyboard));
    BOOST_CHECK(devices == v3d::input::DeviceTypes(v3d::input::DeviceType::Mouse));
}

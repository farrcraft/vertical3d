/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/Feature.h>
#include <api/input/DeviceType.h>

#include <boost/test/unit_test.hpp>

/**
 * What an app asks the engine for. Features combine into a set, and a set accepts only its
 * own enum.
 **/
BOOST_AUTO_TEST_CASE(feature_combine_test) {
    const v3d::engine::Features features = v3d::engine::Feature::Config | v3d::engine::Feature::KeyboardInput;

    BOOST_CHECK(features.has(v3d::engine::Feature::Config));
    BOOST_CHECK(features.has(v3d::engine::Feature::KeyboardInput));
    BOOST_CHECK(!features.has(v3d::engine::Feature::Window));
    BOOST_CHECK(!features.has(v3d::engine::Feature::MouseInput));
}

/**
 * Nothing asked for is nothing enabled, which is the set a test passes when it needs only the
 * asset manager and the event engine.
 **/
BOOST_AUTO_TEST_CASE(feature_none_test) {
    const v3d::engine::Features features;

    BOOST_CHECK(features.empty());
    BOOST_CHECK(!features.has(v3d::engine::Feature::Window));
}

/**
 * A set grows by |=, and a single feature converts to a set of one wherever a set is
 * expected. The engine builds the input engine's device list this way.
 **/
BOOST_AUTO_TEST_CASE(feature_accumulate_test) {
    v3d::input::DeviceTypes devices;
    devices |= v3d::input::DeviceType::Mouse;

    BOOST_CHECK(devices.has(v3d::input::DeviceType::Mouse));
    BOOST_CHECK(!devices.has(v3d::input::DeviceType::Keyboard));
    BOOST_CHECK(devices == v3d::input::DeviceTypes(v3d::input::DeviceType::Mouse));
}

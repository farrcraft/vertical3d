/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/audio/Engine.h>
#include <api/audio/Play.h>
#include <api/log/Logger.h>

#include <limits>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

namespace {

void checkGains(float pan, float left, float right) {
    const v3d::audio::Stereo gains = v3d::audio::stereo(pan);
    BOOST_TEST_CONTEXT("pan " << pan) {
        BOOST_CHECK_EQUAL(gains.left, left);
        BOOST_CHECK_EQUAL(gains.right, right);
    }
}

};  // namespace

BOOST_AUTO_TEST_SUITE(pan_test)

/**
 * A centred sound plays both sides at full gain, exactly as a sound with no pan does.
 **/
BOOST_AUTO_TEST_CASE(a_centred_pan_leaves_both_sides_whole) {
    checkGains(0.0f, 1.0f, 1.0f);
    BOOST_CHECK_EQUAL(v3d::audio::Play().pan, 0.0f);
}

/**
 * The side a sound is panned towards stays at full gain, and the other falls in a straight line
 * to silence at the end of the range.
 **/
BOOST_AUTO_TEST_CASE(a_pan_keeps_its_side_whole_and_fades_the_other) {
    checkGains(-1.0f, 1.0f, 0.0f);
    checkGains(1.0f, 0.0f, 1.0f);
    checkGains(0.5f, 0.5f, 1.0f);
    checkGains(-0.25f, 1.0f, 0.75f);
}

/**
 * A pan past either end is held at that end, and one that is not a number is centred, so a
 * position converted badly by a game still gives gains between silence and full.
 **/
BOOST_AUTO_TEST_CASE(a_pan_outside_the_range_is_clamped) {
    checkGains(3.0f, 0.0f, 1.0f);
    checkGains(-7.0f, 1.0f, 0.0f);
    checkGains(std::numeric_limits<float>::quiet_NaN(), 1.0f, 1.0f);
    checkGains(std::numeric_limits<float>::infinity(), 1.0f, 1.0f);
}

/**
 * A voice nobody was given takes no pan, with or without a device.
 **/
BOOST_AUTO_TEST_CASE(a_pan_without_a_device_finds_no_voice) {
    v3d::audio::Engine sound(boost::make_shared<v3d::log::Logger>(), boost::make_shared<entt::dispatcher>());
    BOOST_TEST(!sound.pan(0, 0.5f));
    BOOST_TEST(!sound.pan(1, -0.5f));
}

BOOST_AUTO_TEST_SUITE_END()

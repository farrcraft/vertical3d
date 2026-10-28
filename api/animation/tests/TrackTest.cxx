/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/animation/Track.h>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/vec4.hpp>

using v3d::animation::Track;

namespace {

BOOST_AUTO_TEST_SUITE(track_test)

/**
 * A track gives each key at its time, the lerp between two keys, and its end keys outside them.
 **/
BOOST_AUTO_TEST_CASE(track_keys_and_lerps_test) {
    const Track<float> track({{1.0F, 2.0F}, {2.0F, 4.0F}, {4.0F, 0.0F}});

    BOOST_CHECK_EQUAL(track.sample(1.0F), 2.0F);
    BOOST_CHECK_EQUAL(track.sample(2.0F), 4.0F);
    BOOST_CHECK_EQUAL(track.sample(4.0F), 0.0F);
    BOOST_CHECK_EQUAL(track.sample(1.5F), 3.0F);
    BOOST_CHECK_EQUAL(track.sample(3.0F), 2.0F);
    BOOST_CHECK_EQUAL(track.sample(0.0F), 2.0F);
    BOOST_CHECK_EQUAL(track.sample(9.0F), 0.0F);
}

/**
 * A colour is lerped a component at a time, as a particle fading out needs.
 **/
BOOST_AUTO_TEST_CASE(track_a_colour_test) {
    const Track<glm::vec4> fade({{0.0F, glm::vec4(1.0F, 1.0F, 1.0F, 1.0F)}, {1.0F, glm::vec4(1.0F, 0.5F, 0.0F, 0.0F)}});

    BOOST_CHECK(fade.sample(0.5F) == glm::vec4(1.0F, 0.75F, 0.5F, 0.5F));
    BOOST_CHECK(Track<glm::vec4>(glm::vec4(0.25F)).sample(7.0F) == glm::vec4(0.25F));
}

/**
 * A wrapping track leads from its last key into its first across the period, and wraps a time
 * outside the period - a day whose midnight is between its last key and its first.
 **/
BOOST_AUTO_TEST_CASE(track_a_wrapping_track_test) {
    const Track<float> day({{6.0F, 1.0F}, {18.0F, 0.0F}}, 24.0F);

    BOOST_CHECK_EQUAL(day.sample(12.0F), 0.5F);
    BOOST_CHECK_EQUAL(day.sample(18.0F), 0.0F);
    // eighteen to six is twelve hours across midnight, which is half way
    BOOST_CHECK_EQUAL(day.sample(0.0F), 0.5F);
    BOOST_CHECK_EQUAL(day.sample(21.0F), 0.25F);
    BOOST_CHECK_EQUAL(day.sample(3.0F), 0.75F);
    BOOST_CHECK_EQUAL(day.sample(36.0F), 0.5F);
    BOOST_CHECK_EQUAL(day.sample(-3.0F), 0.25F);
}

/**
 * A time that is not finite reads as the first key, wrapping or not, rather than searching
 * past the last key.
 **/
BOOST_AUTO_TEST_CASE(track_a_time_that_is_not_finite_test) {
    const Track<float> once({{1.0F, 2.0F}, {2.0F, 4.0F}});
    const Track<float> day({{6.0F, 1.0F}, {18.0F, 0.0F}}, 24.0F);
    for (const float time : {std::nanf(""), std::numeric_limits<float>::infinity()}) {
        BOOST_CHECK_EQUAL(once.sample(time), 2.0F);
        BOOST_CHECK_EQUAL(day.sample(time), 1.0F);
    }
}

/**
 * Keys out of order, or outside a wrapping track's period, are refused, as is a track of none.
 **/
BOOST_AUTO_TEST_CASE(track_refuses_keys_it_cannot_sample_test) {
    const std::vector<Track<float>::Key> none;
    BOOST_CHECK_THROW(Track<float>{none}, std::invalid_argument);
    BOOST_CHECK_THROW(Track<float>({{1.0F, 0.0F}, {1.0F, 1.0F}}), std::invalid_argument);
    BOOST_CHECK_THROW(Track<float>({{2.0F, 0.0F}, {1.0F, 1.0F}}), std::invalid_argument);
    BOOST_CHECK_THROW(Track<float>({{0.0F, 0.0F}, {24.0F, 1.0F}}, 24.0F), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

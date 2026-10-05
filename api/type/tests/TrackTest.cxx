/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/animation/Track.h>

#include <stdexcept>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/vec4.hpp>

using v3d::type::animation::Track;

BOOST_AUTO_TEST_SUITE(track_test)

/**
 * A track gives each key at its time, the lerp between two keys, and its end keys outside them.
 **/
BOOST_AUTO_TEST_CASE(track_keys_and_lerps_test) {
    const Track<float> track({{1.0f, 2.0f}, {2.0f, 4.0f}, {4.0f, 0.0f}});

    BOOST_CHECK_EQUAL(track.sample(1.0f), 2.0f);
    BOOST_CHECK_EQUAL(track.sample(2.0f), 4.0f);
    BOOST_CHECK_EQUAL(track.sample(4.0f), 0.0f);
    BOOST_CHECK_EQUAL(track.sample(1.5f), 3.0f);
    BOOST_CHECK_EQUAL(track.sample(3.0f), 2.0f);
    BOOST_CHECK_EQUAL(track.sample(0.0f), 2.0f);
    BOOST_CHECK_EQUAL(track.sample(9.0f), 0.0f);
}

/**
 * A colour is lerped a component at a time, as a particle fading out needs.
 **/
BOOST_AUTO_TEST_CASE(track_a_colour_test) {
    const Track<glm::vec4> fade({{0.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)}, {1.0f, glm::vec4(1.0f, 0.5f, 0.0f, 0.0f)}});

    BOOST_CHECK(fade.sample(0.5f) == glm::vec4(1.0f, 0.75f, 0.5f, 0.5f));
    BOOST_CHECK(Track<glm::vec4>(glm::vec4(0.25f)).sample(7.0f) == glm::vec4(0.25f));
}

/**
 * A wrapping track leads from its last key into its first across the period, and wraps a time
 * outside the period - a day whose midnight is between its last key and its first.
 **/
BOOST_AUTO_TEST_CASE(track_a_wrapping_track_test) {
    const Track<float> day({{6.0f, 1.0f}, {18.0f, 0.0f}}, 24.0f);

    BOOST_CHECK_EQUAL(day.sample(12.0f), 0.5f);
    BOOST_CHECK_EQUAL(day.sample(18.0f), 0.0f);
    // eighteen to six is twelve hours across midnight, which is half way
    BOOST_CHECK_EQUAL(day.sample(0.0f), 0.5f);
    BOOST_CHECK_EQUAL(day.sample(21.0f), 0.25f);
    BOOST_CHECK_EQUAL(day.sample(3.0f), 0.75f);
    BOOST_CHECK_EQUAL(day.sample(36.0f), 0.5f);
    BOOST_CHECK_EQUAL(day.sample(-3.0f), 0.25f);
}

/**
 * Keys out of order, or outside a wrapping track's period, are refused, as is a track of none.
 **/
BOOST_AUTO_TEST_CASE(track_refuses_keys_it_cannot_sample_test) {
    const std::vector<Track<float>::Key> none;
    BOOST_CHECK_THROW(Track<float>{none}, std::invalid_argument);
    BOOST_CHECK_THROW(Track<float>({{1.0f, 0.0f}, {1.0f, 1.0f}}), std::invalid_argument);
    BOOST_CHECK_THROW(Track<float>({{2.0f, 0.0f}, {1.0f, 1.0f}}), std::invalid_argument);
    BOOST_CHECK_THROW(Track<float>({{0.0f, 0.0f}, {24.0f, 1.0f}}, 24.0f), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

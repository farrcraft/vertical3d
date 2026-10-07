/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <moya/libmoya/Footprint.h>

#include <array>
#include <cmath>
#include <limits>

#include <boost/test/unit_test.hpp>

namespace {

const float infinity = std::numeric_limits<float>::infinity();

/**
 * Whether a footprint touches no pixel.
 **/
bool empty(const std::array<int, 4> & pixels) {
    return pixels[2] < pixels[0] || pixels[3] < pixels[1];
}

};  // namespace

/**
 * A bound inside the frame touches every pixel it overlaps, including the ones it only enters.
 **/
BOOST_AUTO_TEST_CASE(footprint_bound_in_the_frame_test) {
    const std::array<int, 4> pixels = v3d::moya::footprint(glm::vec3(1.5f, 2.25f, 0.0f), glm::vec3(4.0f, 3.5f, 0.0f), 8, 6);
    BOOST_CHECK_EQUAL(pixels[0], 1);
    BOOST_CHECK_EQUAL(pixels[1], 2);
    BOOST_CHECK_EQUAL(pixels[2], 4);
    BOOST_CHECK_EQUAL(pixels[3], 3);
}

/**
 * A bound with a NaN in any of its x and y touches no pixel.
 **/
BOOST_AUTO_TEST_CASE(footprint_nan_bound_touches_nothing_test) {
    const float nan = std::nanf("");
    BOOST_CHECK(empty(v3d::moya::footprint(glm::vec3(nan, 1.0f, 0.0f), glm::vec3(4.0f, 4.0f, 0.0f), 8, 6)));
    BOOST_CHECK(empty(v3d::moya::footprint(glm::vec3(1.0f, nan, 0.0f), glm::vec3(4.0f, 4.0f, 0.0f), 8, 6)));
    BOOST_CHECK(empty(v3d::moya::footprint(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(nan, 4.0f, 0.0f), 8, 6)));
    BOOST_CHECK(empty(v3d::moya::footprint(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(4.0f, nan, 0.0f), 8, 6)));
}

/**
 * An infinite bound covers the whole frame, and one infinite on the wrong side covers none of it.
 **/
BOOST_AUTO_TEST_CASE(footprint_infinite_bound_test) {
    const std::array<int, 4> whole = v3d::moya::footprint(glm::vec3(-infinity, -infinity, 0.0f),
        glm::vec3(infinity, infinity, 0.0f), 8, 6);
    BOOST_CHECK_EQUAL(whole[0], 0);
    BOOST_CHECK_EQUAL(whole[1], 0);
    BOOST_CHECK_EQUAL(whole[2], 7);
    BOOST_CHECK_EQUAL(whole[3], 5);

    BOOST_CHECK(empty(v3d::moya::footprint(glm::vec3(infinity, 0.0f, 0.0f), glm::vec3(infinity, 4.0f, 0.0f), 8, 6)));
    BOOST_CHECK(empty(v3d::moya::footprint(glm::vec3(0.0f, -infinity, 0.0f), glm::vec3(4.0f, -infinity, 0.0f), 8, 6)));
}

/**
 * A bound far off the frame, beyond what an int holds, touches no pixel. One that spans the
 * frame from far off on both sides touches all of it.
 **/
BOOST_AUTO_TEST_CASE(footprint_far_off_bound_test) {
    BOOST_CHECK(empty(v3d::moya::footprint(glm::vec3(1.0e20f, 1.0f, 0.0f), glm::vec3(2.0e20f, 4.0f, 0.0f), 8, 6)));
    BOOST_CHECK(empty(v3d::moya::footprint(glm::vec3(1.0f, -2.0e20f, 0.0f), glm::vec3(4.0f, -1.0e20f, 0.0f), 8, 6)));

    const std::array<int, 4> spans = v3d::moya::footprint(glm::vec3(-1.0e20f, 2.0f, 0.0f),
        glm::vec3(1.0e20f, 3.0f, 0.0f), 8, 6);
    BOOST_CHECK_EQUAL(spans[0], 0);
    BOOST_CHECK_EQUAL(spans[2], 7);
    BOOST_CHECK_EQUAL(spans[1], 2);
    BOOST_CHECK_EQUAL(spans[3], 3);
}

/**
 * A frame with no pixels gives no pixel to touch.
 **/
BOOST_AUTO_TEST_CASE(footprint_empty_frame_test) {
    BOOST_CHECK(empty(v3d::moya::footprint(glm::vec3(0.0f), glm::vec3(4.0f), 0, 6)));
    BOOST_CHECK(empty(v3d::moya::footprint(glm::vec3(0.0f), glm::vec3(4.0f), 8, -2)));
}

/**
 * A time in the shutter falls in the slice that holds it.
 **/
BOOST_AUTO_TEST_CASE(shutter_slice_in_the_shutter_test) {
    const glm::vec2 shutter(0.0f, 1.0f);
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(0.0f, shutter, 8), 0u);
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(0.3f, shutter, 8), 2u);
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(0.99f, shutter, 8), 7u);
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(1.0f, shutter, 8), 7u);
}

/**
 * A NaN time, a time before the shutter and -infinity take the first slice. A time after it
 * and +infinity take the last.
 **/
BOOST_AUTO_TEST_CASE(shutter_slice_outside_the_shutter_test) {
    const glm::vec2 shutter(0.0f, 1.0f);
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(std::nanf(""), shutter, 8), 0u);
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(-0.5f, shutter, 8), 0u);
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(-1.0e30f, shutter, 8), 0u);
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(-infinity, shutter, 8), 0u);
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(1.0e30f, shutter, 8), 7u);
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(infinity, shutter, 8), 7u);
}

/**
 * A shutter that does not open, or one with a NaN or an infinite end, puts every time in the
 * first slice. A count of no slices does too. The largest count is held within its range.
 **/
BOOST_AUTO_TEST_CASE(shutter_slice_degenerate_shutter_test) {
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(0.5f, glm::vec2(1.0f, 1.0f), 8), 0u);
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(0.5f, glm::vec2(1.0f, 0.0f), 8), 0u);
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(0.5f, glm::vec2(std::nanf(""), 1.0f), 8), 0u);
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(0.5f, glm::vec2(-infinity, 1.0f), 8), 0u);
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(1.0e30f, glm::vec2(0.0f, infinity), 8), 0u);
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(infinity, glm::vec2(0.0f, infinity), 8), 0u);
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(0.5f, glm::vec2(0.0f, 1.0f), 0), 0u);
    const unsigned int most = std::numeric_limits<unsigned int>::max();
    BOOST_CHECK_EQUAL(v3d::moya::shutterSlice(infinity, glm::vec2(0.0f, 1.0f), most), most - 1);
}

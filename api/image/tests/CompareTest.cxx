/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Compare.h>

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

boost::shared_ptr<v3d::image::Image> solid(uint32_t width, uint32_t height, uint8_t bpp, unsigned char value) {
    boost::shared_ptr<v3d::image::Image> image = boost::make_shared<v3d::image::Image>(width, height, bpp);
    for (uint32_t i = 0; i < width * height * (bpp / 8u); i++) {
        (*image)[i] = value;
    }
    return image;
}

};  // namespace

BOOST_AUTO_TEST_CASE(compare_identical_test) {
    const boost::shared_ptr<v3d::image::Image> a = solid(2, 2, 24, 128);
    const boost::shared_ptr<v3d::image::Image> b = solid(2, 2, 24, 128);

    v3d::image::Difference difference = v3d::image::compare(*a, *b, 0);
    BOOST_CHECK_EQUAL(difference.match, true);
    BOOST_CHECK_EQUAL(difference.delta, 0u);
    BOOST_CHECK(difference.reason.empty());
}

BOOST_AUTO_TEST_CASE(compare_dimensions_test) {
    const boost::shared_ptr<v3d::image::Image> a = solid(2, 2, 24, 0);
    const boost::shared_ptr<v3d::image::Image> b = solid(2, 3, 24, 0);

    v3d::image::Difference difference = v3d::image::compare(*a, *b, 255);
    // a tolerance wide enough to admit anything still does not admit a different picture
    BOOST_CHECK_EQUAL(difference.match, false);
    BOOST_CHECK(difference.reason.contains("dimensions"));
}

BOOST_AUTO_TEST_CASE(compare_format_test) {
    const boost::shared_ptr<v3d::image::Image> a = solid(2, 2, 24, 0);
    const boost::shared_ptr<v3d::image::Image> b = solid(2, 2, 32, 0);

    v3d::image::Difference difference = v3d::image::compare(*a, *b, 255);
    BOOST_CHECK_EQUAL(difference.match, false);
    BOOST_CHECK(difference.reason.contains("formats"));
}

BOOST_AUTO_TEST_CASE(compare_tolerance_test) {
    const boost::shared_ptr<v3d::image::Image> a = solid(2, 2, 24, 100);
    const boost::shared_ptr<v3d::image::Image> b = solid(2, 2, 24, 100);
    // the third channel of the pixel at column 1, row 1
    (*b)[11] = 102;

    // a difference of exactly the tolerance is a match
    v3d::image::Difference within = v3d::image::compare(*a, *b, 2);
    BOOST_CHECK_EQUAL(within.match, true);

    // and one above it is not
    v3d::image::Difference beyond = v3d::image::compare(*a, *b, 1);
    BOOST_CHECK_EQUAL(beyond.match, false);
}

BOOST_AUTO_TEST_CASE(compare_worst_pixel_test) {
    const boost::shared_ptr<v3d::image::Image> a = solid(2, 2, 24, 100);
    const boost::shared_ptr<v3d::image::Image> b = solid(2, 2, 24, 100);
    // two disagreements, so the report has to name the larger one
    (*b)[1] = 105;
    (*b)[10] = 60;

    v3d::image::Difference difference = v3d::image::compare(*a, *b, 0);
    BOOST_CHECK_EQUAL(difference.match, false);
    BOOST_CHECK_EQUAL(difference.delta, 40u);
    BOOST_CHECK_EQUAL(difference.column, 1u);
    BOOST_CHECK_EQUAL(difference.row, 1u);
    BOOST_CHECK_EQUAL(difference.channel, 1u);

    // and says so in one line, rather than only that the images differed
    BOOST_CHECK(difference.description().contains("column 1"));
    BOOST_CHECK(difference.description().contains("row 1"));
}

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(image_test) {
    v3d::image::Image img24(2, 4, 24);
    BOOST_CHECK_EQUAL((img24.format() == v3d::image::Image::Format::RGB), true);

    v3d::image::Image img32(1, 5, 32);
    BOOST_CHECK_EQUAL((img32.format() == v3d::image::Image::Format::RGBA), true);

    BOOST_CHECK_EQUAL(img24.width(), 2u);
    BOOST_CHECK_EQUAL(img24.height(), 4u);
    BOOST_CHECK_EQUAL(img24.bpp(), 24u);

    img32.width(3);
    BOOST_CHECK_EQUAL(img32.width(), 3u);

    img32.height(7);
    BOOST_CHECK_EQUAL(img32.height(), 7u);

    img32.bpp(24);
    BOOST_CHECK_EQUAL(img32.bpp(), 24u);

    // the length constructor allocates without deciding on a shape
    v3d::image::Image img(static_cast<uint64_t>(12));
    img.width(2);
    img.height(2);
    img.bpp(24);

    // pixel data starts zeroed
    BOOST_CHECK_EQUAL(img[3], 0);
    img[0] = 3;
    BOOST_CHECK_EQUAL(img[0], 3);

    // and data() is the same storage the subscript reaches
    BOOST_CHECK_EQUAL(img.data()[0], 3);
}

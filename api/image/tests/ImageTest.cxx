/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Channels.h>
#include <api/image/Image.h>

#include <type_traits>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(image_test) {
    v3d::image::Image img24(2, 4, 24);
    BOOST_CHECK_EQUAL((img24.format() == v3d::image::Image::Format::RGB), true);

    v3d::image::Image img32(1, 5, 32);
    BOOST_CHECK_EQUAL((img32.format() == v3d::image::Image::Format::RGBA), true);

    BOOST_CHECK_EQUAL(img24.width(), 2u);
    BOOST_CHECK_EQUAL(img24.height(), 4u);
    BOOST_CHECK_EQUAL(img24.bpp(), 24u);

    // an image's shape is what it was made with, since the buffer was sized for it
    v3d::image::Image img(2, 2, 24);

    // pixel data starts zeroed
    BOOST_CHECK_EQUAL(img[3], 0);
    img[0] = 3;
    BOOST_CHECK_EQUAL(img[0], 3);

    // and data() is the same storage the subscript reaches
    BOOST_CHECK_EQUAL(img.data()[0], 3);
}

/**
 * An image owns its buffer and frees it, so the compiler refuses a copy rather than the
 * program freeing it twice. crop() is how a copy is actually made.
 *
 * Stated here rather than in a case, because the check is the build: a copy constructor put
 * back by hand fails to compile at this line instead of failing a run somewhere else.
 **/
static_assert(!std::is_copy_constructible<v3d::image::Image>::value,
    "copying an image would free its buffer twice");
static_assert(!std::is_copy_assignable<v3d::image::Image>::value,
    "assigning an image would free its buffer twice");

/**
 * format() is the channel count and it follows the depth. A depth no format describes still
 * gives a defined value, because every writer reads format() to size a row.
 **/
BOOST_AUTO_TEST_CASE(image_format_follows_depth_test) {
    // a texture atlas packed at depth 1 is this
    const v3d::image::Image grey(4, 4, 8);
    BOOST_CHECK((grey.format() == v3d::image::Image::Format::Grey));

    const v3d::image::Image colour(2, 2, 24);
    BOOST_CHECK((colour.format() == v3d::image::Image::Format::RGB));
    const v3d::image::Image alpha(2, 2, 32);
    BOOST_CHECK((alpha.format() == v3d::image::Image::Format::RGBA));
}

/**
 * The swap BMP and TGA go through exchanges red and blue and carries an alpha across as it
 * is, in place or into another buffer.
 **/
BOOST_AUTO_TEST_CASE(image_swap_red_blue_test) {
    const unsigned char pixels[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    unsigned char swapped[8] = {};
    v3d::image::swapRedBlue(pixels, swapped, 2, 4);
    const unsigned char expected[8] = { 3, 2, 1, 4, 7, 6, 5, 8 };
    BOOST_CHECK_EQUAL_COLLECTIONS(swapped, swapped + 8, expected, expected + 8);

    unsigned char inPlace[6] = { 1, 2, 3, 4, 5, 6 };
    v3d::image::swapRedBlue(inPlace, inPlace, 2, 3);
    const unsigned char turned[6] = { 3, 2, 1, 6, 5, 4 };
    BOOST_CHECK_EQUAL_COLLECTIONS(inPlace, inPlace + 6, turned, turned + 6);
}

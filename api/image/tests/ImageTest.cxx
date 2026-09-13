/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

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
 * format() is the channel count and it follows the depth, rather than being decided once for
 * the two depths that happened to be handled. A depth no format describes still leaves a
 * definite answer, because an indeterminate one is what every writer reads to size a row.
 **/
BOOST_AUTO_TEST_CASE(image_format_follows_depth_test) {
    // a texture atlas packed at depth 1 and a Font2D bitmap are both this
    const v3d::image::Image grey(4, 4, 8);
    BOOST_CHECK((grey.format() == v3d::image::Image::Format::Grey));

    // the length constructor decides no shape at all and still has a definite format
    const v3d::image::Image blob(static_cast<uint64_t>(8));
    BOOST_CHECK((blob.format() == v3d::image::Image::Format::RGB));

    // and setting the depth keeps the two in step
    v3d::image::Image image(2, 2, 32);
    BOOST_CHECK((image.format() == v3d::image::Image::Format::RGBA));
    image.bpp(8);
    BOOST_CHECK((image.format() == v3d::image::Image::Format::Grey));
    image.bpp(24);
    BOOST_CHECK((image.format() == v3d::image::Image::Format::RGB));
}

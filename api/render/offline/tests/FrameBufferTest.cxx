/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/FrameBuffer.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(framebuffer_plane_test) {
    v3d::render::offline::FrameBuffer buffer(4, 3, 4);

    BOOST_CHECK_EQUAL(buffer.width(), 4u);
    BOOST_CHECK_EQUAL(buffer.height(), 3u);
    BOOST_CHECK_EQUAL(buffer.planes(), 4u);

    // every plane starts at zero
    BOOST_CHECK_EQUAL(buffer.value(2, 3, 2), 0.0f);

    buffer.value(2, 3, 2, 0.5f);
    BOOST_CHECK_EQUAL(buffer.value(2, 3, 2), 0.5f);
    // and the write lands on one pixel of one plane
    BOOST_CHECK_EQUAL(buffer.value(1, 3, 2), 0.0f);
    BOOST_CHECK_EQUAL(buffer.value(2, 2, 2), 0.0f);
    BOOST_CHECK_EQUAL(buffer.value(2, 3, 1), 0.0f);

    buffer.clear(3, 1.0f);
    BOOST_CHECK_EQUAL(buffer.value(3, 0, 0), 1.0f);
    BOOST_CHECK_EQUAL(buffer.value(3, 3, 2), 1.0f);
    BOOST_CHECK_EQUAL(buffer.value(2, 0, 0), 0.0f);
}

BOOST_AUTO_TEST_CASE(framebuffer_image_test) {
    v3d::render::offline::FrameBuffer buffer(2, 2, 4);

    buffer.value(0, 0, 0, 1.0f);
    buffer.value(1, 1, 0, 1.0f);
    buffer.value(2, 0, 1, 1.0f);
    buffer.value(3, 1, 1, 1.0f);

    auto image = buffer.image(4);
    BOOST_CHECK_EQUAL(image->width(), 2u);
    BOOST_CHECK_EQUAL(image->height(), 2u);
    BOOST_CHECK_EQUAL(image->bpp(), 32u);

    // 1.0 is 255, and each write reaches its own channel of its own pixel
    BOOST_CHECK_EQUAL((*image)[0], 255);
    BOOST_CHECK_EQUAL((*image)[1], 0);
    BOOST_CHECK_EQUAL((*image)[5], 255);
    BOOST_CHECK_EQUAL((*image)[10], 255);
    BOOST_CHECK_EQUAL((*image)[15], 255);
}

BOOST_AUTO_TEST_CASE(framebuffer_row_order_test) {
    v3d::render::offline::FrameBuffer buffer(2, 2, 3);

    // row 0 of the buffer is row 0 of the image, which is the top of the picture
    buffer.value(0, 0, 0, 1.0f);
    buffer.value(1, 0, 1, 1.0f);

    auto image = buffer.image(3);
    BOOST_CHECK_EQUAL((*image)[0], 255);
    BOOST_CHECK_EQUAL((*image)[1], 0);
    // second row, first pixel, green channel
    BOOST_CHECK_EQUAL((*image)[6], 0);
    BOOST_CHECK_EQUAL((*image)[7], 255);
}

BOOST_AUTO_TEST_CASE(framebuffer_saturation_test) {
    v3d::render::offline::FrameBuffer buffer(2, 1, 3);

    // a cast alone wraps a value above one, which turns a bright pixel dark
    buffer.value(0, 0, 0, 4.0f);
    buffer.value(1, 0, 0, 1.004f);
    buffer.value(2, 0, 0, -0.5f);

    auto image = buffer.image(3);
    BOOST_CHECK_EQUAL((*image)[0], 255);
    BOOST_CHECK_EQUAL((*image)[1], 255);
    BOOST_CHECK_EQUAL((*image)[2], 0);
}

BOOST_AUTO_TEST_CASE(framebuffer_extra_plane_test) {
    v3d::render::offline::FrameBuffer buffer(1, 1, 4);

    buffer.value(0, 0, 0, 1.0f);
    buffer.value(1, 0, 0, 1.0f);
    buffer.value(2, 0, 0, 1.0f);
    // the fourth plane is a depth here, not an alpha
    buffer.value(3, 0, 0, 0.25f);

    // three channels asked for, so the depth is not in the picture
    auto image = buffer.image(3);
    BOOST_CHECK_EQUAL(image->bpp(), 24u);
    BOOST_CHECK_EQUAL((*image)[0], 255);
    BOOST_CHECK_EQUAL((*image)[1], 255);
    BOOST_CHECK_EQUAL((*image)[2], 255);
}

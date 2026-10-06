/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/vulkan/renderer/Clip.h>

#include <limits>

#include <boost/test/unit_test.hpp>
#include <glm/vec4.hpp>

using v3d::render::realtime::DrawItem;
using v3d::render::realtime::vulkan::renderer::clip;

BOOST_AUTO_TEST_SUITE(clip_test)

/**
 * A rectangle inside the image becomes a scissor of the same pixels, and marks the item cut.
 **/
BOOST_AUTO_TEST_CASE(a_rectangle_inside_the_image_is_its_scissor) {
    DrawItem item;
    clip(&item, glm::vec4(4.0f, 8.0f, 20.0f, 12.0f));

    BOOST_CHECK(item.scissored);
    BOOST_CHECK_EQUAL(item.scissor.offset.x, 4);
    BOOST_CHECK_EQUAL(item.scissor.offset.y, 8);
    BOOST_CHECK_EQUAL(item.scissor.extent.width, 16u);
    BOOST_CHECK_EQUAL(item.scissor.extent.height, 4u);
}

/**
 * What lies off the top or left of the image is dropped, so the scissor starts at the image's
 * edge and keeps only the part on the image.
 **/
BOOST_AUTO_TEST_CASE(a_negative_origin_is_cut_at_the_edge) {
    DrawItem item;
    clip(&item, glm::vec4(-10.0f, -5.0f, 6.0f, 7.0f));

    BOOST_CHECK_EQUAL(item.scissor.offset.x, 0);
    BOOST_CHECK_EQUAL(item.scissor.offset.y, 0);
    BOOST_CHECK_EQUAL(item.scissor.extent.width, 6u);
    BOOST_CHECK_EQUAL(item.scissor.extent.height, 7u);
}

/**
 * A rectangle wholly off the top left, or turned inside out, cuts the draw to nothing.
 **/
BOOST_AUTO_TEST_CASE(an_inverted_rectangle_draws_nothing) {
    DrawItem inverted;
    clip(&inverted, glm::vec4(20.0f, 30.0f, 10.0f, 5.0f));
    BOOST_CHECK(inverted.scissored);
    BOOST_CHECK_EQUAL(inverted.scissor.extent.width, 0u);
    BOOST_CHECK_EQUAL(inverted.scissor.extent.height, 0u);

    DrawItem off;
    clip(&off, glm::vec4(-20.0f, -20.0f, -10.0f, -10.0f));
    BOOST_CHECK_EQUAL(off.scissor.offset.x, 0);
    BOOST_CHECK_EQUAL(off.scissor.offset.y, 0);
    BOOST_CHECK_EQUAL(off.scissor.extent.width, 0u);
    BOOST_CHECK_EQUAL(off.scissor.extent.height, 0u);
}

/**
 * A NaN left or top edge is the image's edge, and a NaN right or bottom edge cuts the draw to
 * nothing, so no NaN reaches a conversion to an integer.
 **/
BOOST_AUTO_TEST_CASE(a_nan_edge_gives_a_defined_scissor) {
    const float nan = std::numeric_limits<float>::quiet_NaN();

    DrawItem origin;
    clip(&origin, glm::vec4(nan, nan, 8.0f, 6.0f));
    BOOST_CHECK_EQUAL(origin.scissor.offset.x, 0);
    BOOST_CHECK_EQUAL(origin.scissor.offset.y, 0);
    BOOST_CHECK_EQUAL(origin.scissor.extent.width, 8u);
    BOOST_CHECK_EQUAL(origin.scissor.extent.height, 6u);

    DrawItem extent;
    clip(&extent, glm::vec4(2.0f, 3.0f, nan, nan));
    BOOST_CHECK_EQUAL(extent.scissor.offset.x, 2);
    BOOST_CHECK_EQUAL(extent.scissor.offset.y, 3);
    BOOST_CHECK_EQUAL(extent.scissor.extent.width, 0u);
    BOOST_CHECK_EQUAL(extent.scissor.extent.height, 0u);
}

/**
 * An edge past any image, infinity included, is held to a bound that converts to an integer
 * exactly.
 **/
BOOST_AUTO_TEST_CASE(an_unbounded_rectangle_is_held_in_range) {
    const float infinity = std::numeric_limits<float>::infinity();

    DrawItem item;
    clip(&item, glm::vec4(-infinity, 1.0f, infinity, 1.0e30f));
    BOOST_CHECK_EQUAL(item.scissor.offset.x, 0);
    BOOST_CHECK_EQUAL(item.scissor.offset.y, 1);
    BOOST_CHECK_EQUAL(item.scissor.extent.width, 16777216u);
    BOOST_CHECK_EQUAL(item.scissor.extent.height, 16777215u);
}

BOOST_AUTO_TEST_SUITE_END()

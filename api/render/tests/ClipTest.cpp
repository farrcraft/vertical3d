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

namespace {

BOOST_AUTO_TEST_SUITE(clip_test)

/**
 * A rectangle inside the image becomes a scissor of the same pixels, and marks the item cut.
 **/
BOOST_AUTO_TEST_CASE(a_rectangle_inside_the_image_is_its_scissor) {
    DrawItem item;
    clip(&item, glm::vec4(4.0F, 8.0F, 20.0F, 12.0F));

    BOOST_CHECK(item.scissored);
    BOOST_CHECK_EQUAL(item.scissor.offset.x, 4);
    BOOST_CHECK_EQUAL(item.scissor.offset.y, 8);
    BOOST_CHECK_EQUAL(item.scissor.extent.width, 16U);
    BOOST_CHECK_EQUAL(item.scissor.extent.height, 4U);
}

/**
 * What lies off the top or left of the image is dropped, so the scissor starts at the image's
 * edge and keeps only the part on the image.
 **/
BOOST_AUTO_TEST_CASE(a_negative_origin_is_cut_at_the_edge) {
    DrawItem item;
    clip(&item, glm::vec4(-10.0F, -5.0F, 6.0F, 7.0F));

    BOOST_CHECK_EQUAL(item.scissor.offset.x, 0);
    BOOST_CHECK_EQUAL(item.scissor.offset.y, 0);
    BOOST_CHECK_EQUAL(item.scissor.extent.width, 6U);
    BOOST_CHECK_EQUAL(item.scissor.extent.height, 7U);
}

/**
 * A rectangle wholly off the top left, or turned inside out, cuts the draw to nothing.
 **/
BOOST_AUTO_TEST_CASE(an_inverted_rectangle_draws_nothing) {
    DrawItem inverted;
    clip(&inverted, glm::vec4(20.0F, 30.0F, 10.0F, 5.0F));
    BOOST_CHECK(inverted.scissored);
    BOOST_CHECK_EQUAL(inverted.scissor.extent.width, 0U);
    BOOST_CHECK_EQUAL(inverted.scissor.extent.height, 0U);

    DrawItem off;
    clip(&off, glm::vec4(-20.0F, -20.0F, -10.0F, -10.0F));
    BOOST_CHECK_EQUAL(off.scissor.offset.x, 0);
    BOOST_CHECK_EQUAL(off.scissor.offset.y, 0);
    BOOST_CHECK_EQUAL(off.scissor.extent.width, 0U);
    BOOST_CHECK_EQUAL(off.scissor.extent.height, 0U);
}

/**
 * A NaN left or top edge is the image's edge, and a NaN right or bottom edge cuts the draw to
 * nothing, so no NaN reaches a conversion to an integer.
 **/
BOOST_AUTO_TEST_CASE(a_nan_edge_gives_a_defined_scissor) {
    const float nan = std::numeric_limits<float>::quiet_NaN();

    DrawItem origin;
    clip(&origin, glm::vec4(nan, nan, 8.0F, 6.0F));
    BOOST_CHECK_EQUAL(origin.scissor.offset.x, 0);
    BOOST_CHECK_EQUAL(origin.scissor.offset.y, 0);
    BOOST_CHECK_EQUAL(origin.scissor.extent.width, 8U);
    BOOST_CHECK_EQUAL(origin.scissor.extent.height, 6U);

    DrawItem extent;
    clip(&extent, glm::vec4(2.0F, 3.0F, nan, nan));
    BOOST_CHECK_EQUAL(extent.scissor.offset.x, 2);
    BOOST_CHECK_EQUAL(extent.scissor.offset.y, 3);
    BOOST_CHECK_EQUAL(extent.scissor.extent.width, 0U);
    BOOST_CHECK_EQUAL(extent.scissor.extent.height, 0U);
}

/**
 * An edge past any image, infinity included, is held to a bound that converts to an integer
 * exactly.
 **/
BOOST_AUTO_TEST_CASE(an_unbounded_rectangle_is_held_in_range) {
    const float infinity = std::numeric_limits<float>::infinity();

    DrawItem item;
    clip(&item, glm::vec4(-infinity, 1.0F, infinity, 1.0e30F));
    BOOST_CHECK_EQUAL(item.scissor.offset.x, 0);
    BOOST_CHECK_EQUAL(item.scissor.offset.y, 1);
    BOOST_CHECK_EQUAL(item.scissor.extent.width, 16777216U);
    BOOST_CHECK_EQUAL(item.scissor.extent.height, 16777215U);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

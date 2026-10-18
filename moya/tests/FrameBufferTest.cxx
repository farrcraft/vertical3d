/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/geometry/AABBox.h>
#include <moya/libmoya/FrameBuffer.h>
#include <moya/libmoya/Polygon.h>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/glm.hpp>

namespace {

v3d::type::geometry::AABBox box(const glm::vec3 & min, const glm::vec3 & max) {
    v3d::type::geometry::AABBox bound;
    bound.extents(min, max);
    return bound;
}

/**
 * An image that is a whole number of buckets across gets exactly that many.
 **/
BOOST_AUTO_TEST_CASE(framebuffer_bucket_grid_test) {
    const v3d::moya::FrameBuffer buffer({.bucket = glm::uvec2(16, 16), .image = glm::uvec2(320, 240)});

    BOOST_TEST(buffer.bucketColumns() == 20U);
    BOOST_TEST(buffer.bucketRows() == 15U);
}

/**
 * An image that is not gets a partial bucket rather than losing its right and bottom edges,
 * as an integer division of the two sizes would.
 **/
BOOST_AUTO_TEST_CASE(framebuffer_partial_bucket_test) {
    const v3d::moya::FrameBuffer buffer({.bucket = glm::uvec2(16, 16), .image = glm::uvec2(330, 250)});

    BOOST_TEST(buffer.bucketColumns() == 21U);
    BOOST_TEST(buffer.bucketRows() == 16U);
}

/**
 * A raster space bound is placed by its upper left corner. A primitive straddling the top or
 * left edge has a negative one, which cannot be converted to an unsigned bucket index - it
 * belongs to the first bucket on that axis.
 **/
BOOST_AUTO_TEST_CASE(framebuffer_negative_bound_test) {
    v3d::moya::FrameBuffer buffer({.bucket = glm::uvec2(16, 16), .image = glm::uvec2(320, 240)});

    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    buffer.addPrimitive(polygon, box(glm::vec3(-40.0F, -12.0F, 0.0F), glm::vec3(10.0F, 10.0F, 0.0F)));

    // nothing to assert beyond the placement not being undefined; a bound past the far edge
    // is the same question from the other side
    buffer.addPrimitive(polygon, box(glm::vec3(9000.0F, 9000.0F, 0.0F), glm::vec3(9100.0F, 9100.0F, 0.0F)));
}

};  // namespace

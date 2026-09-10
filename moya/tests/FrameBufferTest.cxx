/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <moya/libmoya/FrameBuffer.h>
#include <moya/libmoya/Polygon.h>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/glm.hpp>

namespace {

v3d::type::geometry::AABBox box(const glm::vec3 & min, const glm::vec3 & max) {
    v3d::type::geometry::AABBox b;
    b.extents(min, max);
    return b;
}

};  // namespace

/**
 * An image that is a whole number of buckets across gets exactly that many.
 **/
BOOST_AUTO_TEST_CASE(framebuffer_bucket_grid_test) {
    unsigned int bucket[2] = { 16, 16 };
    unsigned int image[2] = { 320, 240 };
    v3d::moya::FrameBuffer buffer(bucket, image);

    BOOST_TEST(buffer.bucketColumns() == 20u);
    BOOST_TEST(buffer.bucketRows() == 15u);
}

/**
 * An image that is not gets a partial bucket rather than losing its right and bottom edges,
 * which an integer division of the two sizes silently did.
 **/
BOOST_AUTO_TEST_CASE(framebuffer_partial_bucket_test) {
    unsigned int bucket[2] = { 16, 16 };
    unsigned int image[2] = { 330, 250 };
    v3d::moya::FrameBuffer buffer(bucket, image);

    BOOST_TEST(buffer.bucketColumns() == 21u);
    BOOST_TEST(buffer.bucketRows() == 16u);
}

/**
 * A raster space bound is placed by its upper left corner. A primitive straddling the top or
 * left edge has a negative one, which cannot be converted to an unsigned bucket index - it
 * belongs to the first bucket on that axis.
 **/
BOOST_AUTO_TEST_CASE(framebuffer_negative_bound_test) {
    unsigned int bucket[2] = { 16, 16 };
    unsigned int image[2] = { 320, 240 };
    v3d::moya::FrameBuffer buffer(bucket, image);

    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    buffer.addPrimitive(polygon, box(glm::vec3(-40.0f, -12.0f, 0.0f), glm::vec3(10.0f, 10.0f, 0.0f)));

    // nothing to assert beyond the placement not being undefined; a bound past the far edge
    // is the same question from the other side
    buffer.addPrimitive(polygon, box(glm::vec3(9000.0f, 9000.0f, 0.0f), glm::vec3(9100.0f, 9100.0f, 0.0f)));
}

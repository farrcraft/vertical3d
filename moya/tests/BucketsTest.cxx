/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/geometry/AABBox.h>
#include <moya/libmoya/Polygon.h>
#include <moya/libmoya/reyes/Buckets.h>

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
BOOST_AUTO_TEST_CASE(buckets_grid_test) {
    const v3d::moya::reyes::Buckets buffer({.bucket = glm::uvec2(16, 16), .image = glm::uvec2(320, 240)});

    BOOST_TEST(buffer.columns() == 20U);
    BOOST_TEST(buffer.rows() == 15U);
}

/**
 * An image that is not a whole number of buckets across gets a partial bucket over its right
 * and bottom edges. An integer division of the two sizes would lose those edges.
 **/
BOOST_AUTO_TEST_CASE(buckets_partial_bucket_test) {
    const v3d::moya::reyes::Buckets buffer({.bucket = glm::uvec2(16, 16), .image = glm::uvec2(330, 250)});

    BOOST_TEST(buffer.columns() == 21U);
    BOOST_TEST(buffer.rows() == 16U);
}

/**
 * A raster space bound is placed by its upper left corner. A primitive straddling the top or
 * left edge has a negative corner, and belongs to the first bucket on that axis. A bound past
 * the right or bottom edge belongs to the last bucket on that axis.
 **/
BOOST_AUTO_TEST_CASE(buckets_negative_bound_test) {
    v3d::moya::reyes::Buckets buffer({.bucket = glm::uvec2(16, 16), .image = glm::uvec2(320, 240)});

    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    buffer.add(polygon, box(glm::vec3(-40.0F, -12.0F, 0.0F), glm::vec3(10.0F, 10.0F, 0.0F)));
    BOOST_TEST(buffer.primitiveCount(0, 0) == 1U);

    buffer.add(polygon, box(glm::vec3(9000.0F, 9000.0F, 0.0F), glm::vec3(9100.0F, 9100.0F, 0.0F)));
    BOOST_TEST(buffer.primitiveCount(19, 14) == 1U);
    BOOST_TEST(buffer.primitiveCount() == 2U);
}

};  // namespace

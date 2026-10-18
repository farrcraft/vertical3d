/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <moya/libmoya/Vertex.h>
#include <moya/libmoya/reyes/MicroPolygon.h>
#include <moya/libmoya/reyes/MicroPolygonGrid.h>

#include <array>

#include <boost/test/unit_test.hpp>

#include <glm/glm.hpp>

namespace {

v3d::moya::Vertex vertex(float x, float y, float z) {
    v3d::moya::Vertex v;
    v.point(glm::vec3(x, y, z));
    return v;
}

/**
 * A grid's extent is fixed when it is made, so every point in it can be written to.
 **/
BOOST_AUTO_TEST_CASE(micropolygon_grid_size_test) {
    v3d::moya::reyes::MicroPolygonGrid grid(4);

    BOOST_TEST(grid.size() == 4U);

    // every point in the grid is addressable, including the far corner
    grid.addVertex(vertex(1.0F, 2.0F, 3.0F), 3, 3);
    BOOST_TEST((grid.vertex(3, 3).point() == glm::vec3(1.0F, 2.0F, 3.0F)));

    // and one that has not been written reads back as a default vertex
    BOOST_TEST((grid.vertex(0, 0).point() == glm::vec3(0.0F, 0.0F, 0.0F)));
}

/**
 * A micropolygon is named by its lower indexed corner, and shares its vertices with its
 * neighbours. Its four corners come off the grid in winding order.
 **/
BOOST_AUTO_TEST_CASE(micropolygon_grid_polygon_test) {
    v3d::moya::reyes::MicroPolygonGrid grid(2);
    grid.addVertex(vertex(0.0F, 0.0F, 0.0F), 0, 0);
    grid.addVertex(vertex(0.0F, 1.0F, 0.0F), 0, 1);
    grid.addVertex(vertex(1.0F, 1.0F, 0.0F), 1, 1);
    grid.addVertex(vertex(1.0F, 0.0F, 0.0F), 1, 0);

    v3d::moya::reyes::MicroPolygon poly = grid.microPolygon(0, 0);

    BOOST_TEST((poly[0].point() == glm::vec3(0.0F, 0.0F, 0.0F)));
    BOOST_TEST((poly[1].point() == glm::vec3(0.0F, 1.0F, 0.0F)));
    BOOST_TEST((poly[2].point() == glm::vec3(1.0F, 1.0F, 0.0F)));
    BOOST_TEST((poly[3].point() == glm::vec3(1.0F, 0.0F, 0.0F)));
}

/**
 * A grid of n vertices a side holds n-1 micropolygons a side: the last row and column close
 * the grid rather than opening a polygon of their own.
 **/
BOOST_AUTO_TEST_CASE(micropolygon_grid_extent_test) {
    v3d::moya::reyes::MicroPolygonGrid grid(3);

    BOOST_TEST(grid.size() == 3U);
    // (1, 1) is the last polygon that fits, and reads vertices (1, 1) through (2, 2)
    grid.addVertex(vertex(9.0F, 9.0F, 9.0F), 2, 2);
    BOOST_TEST((grid.microPolygon(1, 1)[2].point() == glm::vec3(9.0F, 9.0F, 9.0F)));
}

/**
 * A micropolygon turned 45 degrees covers the points inside it, and not the corners of its
 * raster bound.
 **/
BOOST_AUTO_TEST_CASE(micropolygon_covers_test) {
    const std::array<glm::vec3, 4> diamond = {
        glm::vec3(2.0F, 0.0F, 1.0F), glm::vec3(4.0F, 2.0F, 1.0F),
        glm::vec3(2.0F, 4.0F, 1.0F), glm::vec3(0.0F, 2.0F, 1.0F)
    };
    float depth = 0.0F;
    BOOST_CHECK(v3d::moya::reyes::covers(diamond, glm::vec2(2.0F, 2.0F), &depth));
    BOOST_CHECK_EQUAL(depth, 1.0F);
    BOOST_CHECK(v3d::moya::reyes::covers(diamond, glm::vec2(2.5F, 0.5F), &depth));
    BOOST_CHECK(v3d::moya::reyes::covers(diamond, glm::vec2(1.0F, 3.0F), &depth));
    // every corner of the bound is outside
    BOOST_CHECK(!v3d::moya::reyes::covers(diamond, glm::vec2(0.5F, 0.5F), &depth));
    BOOST_CHECK(!v3d::moya::reyes::covers(diamond, glm::vec2(3.5F, 0.5F), &depth));
    BOOST_CHECK(!v3d::moya::reyes::covers(diamond, glm::vec2(3.5F, 3.5F), &depth));
    BOOST_CHECK(!v3d::moya::reyes::covers(diamond, glm::vec2(0.5F, 3.5F), &depth));

    // the other winding is the same micropolygon
    const std::array<glm::vec3, 4> reversed = { diamond[3], diamond[2], diamond[1], diamond[0] };
    BOOST_CHECK(v3d::moya::reyes::covers(reversed, glm::vec2(2.0F, 2.0F), &depth));
    BOOST_CHECK(!v3d::moya::reyes::covers(reversed, glm::vec2(0.5F, 0.5F), &depth));

    // and one collapsed to a line covers nothing
    const std::array<glm::vec3, 4> line = {
        glm::vec3(0.0F, 0.0F, 1.0F), glm::vec3(4.0F, 0.0F, 1.0F),
        glm::vec3(4.0F, 0.0F, 1.0F), glm::vec3(0.0F, 0.0F, 1.0F)
    };
    BOOST_CHECK(!v3d::moya::reyes::covers(line, glm::vec2(2.0F, 0.0F), &depth));
}

/**
 * A depth is interpolated at the point rather than averaged over the corners. Two
 * micropolygons over the same square, tilted opposite ways, have the same mean depth, and
 * which is nearer depends on where in the square the sample is.
 **/
BOOST_AUTO_TEST_CASE(micropolygon_depth_is_interpolated_test) {
    const std::array<glm::vec3, 4> rising = {
        glm::vec3(0.0F, 0.0F, 1.0F), glm::vec3(4.0F, 0.0F, 3.0F),
        glm::vec3(4.0F, 4.0F, 3.0F), glm::vec3(0.0F, 4.0F, 1.0F)
    };
    const std::array<glm::vec3, 4> falling = {
        glm::vec3(0.0F, 0.0F, 3.0F), glm::vec3(4.0F, 0.0F, 1.0F),
        glm::vec3(4.0F, 4.0F, 1.0F), glm::vec3(0.0F, 4.0F, 3.0F)
    };
    float risingDepth = 0.0F;
    float fallingDepth = 0.0F;
    BOOST_REQUIRE(v3d::moya::reyes::covers(rising, glm::vec2(1.0F, 2.0F), &risingDepth));
    BOOST_REQUIRE(v3d::moya::reyes::covers(falling, glm::vec2(1.0F, 2.0F), &fallingDepth));
    BOOST_CHECK_CLOSE(risingDepth, 1.5F, 1.0e-4F);
    BOOST_CHECK_CLOSE(fallingDepth, 2.5F, 1.0e-4F);

    BOOST_REQUIRE(v3d::moya::reyes::covers(rising, glm::vec2(3.0F, 2.0F), &risingDepth));
    BOOST_REQUIRE(v3d::moya::reyes::covers(falling, glm::vec2(3.0F, 2.0F), &fallingDepth));
    BOOST_CHECK(fallingDepth < risingDepth);
}

};  // namespace

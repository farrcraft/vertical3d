/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <moya/libmoya/MicroPolygon.h>
#include <moya/libmoya/MicroPolygonGrid.h>

#include <array>

#include <boost/test/unit_test.hpp>

#include <glm/glm.hpp>

namespace {

v3d::moya::Vertex vertex(float x, float y, float z) {
    v3d::moya::Vertex v;
    v.point(glm::vec3(x, y, z));
    return v;
}

};  // namespace

/**
 * A grid's extent is fixed when it is made, so every point in it can be written to. It used
 * to be built empty and indexed into regardless.
 **/
BOOST_AUTO_TEST_CASE(micropolygon_grid_size_test) {
    v3d::moya::MicroPolygonGrid grid(4);

    BOOST_TEST(grid.size() == 4u);

    // every point in the grid is addressable, including the far corner
    grid.addVertex(vertex(1.0f, 2.0f, 3.0f), 3, 3);
    BOOST_TEST((grid.vertex(3, 3).point() == glm::vec3(1.0f, 2.0f, 3.0f)));

    // and one that has not been written reads back as a default vertex
    BOOST_TEST((grid.vertex(0, 0).point() == glm::vec3(0.0f, 0.0f, 0.0f)));
}

/**
 * A micropolygon is named by its lower indexed corner and shares its vertices with its
 * neighbours, so the four corners come off the grid in winding order.
 **/
BOOST_AUTO_TEST_CASE(micropolygon_grid_polygon_test) {
    v3d::moya::MicroPolygonGrid grid(2);
    grid.addVertex(vertex(0.0f, 0.0f, 0.0f), 0, 0);
    grid.addVertex(vertex(0.0f, 1.0f, 0.0f), 0, 1);
    grid.addVertex(vertex(1.0f, 1.0f, 0.0f), 1, 1);
    grid.addVertex(vertex(1.0f, 0.0f, 0.0f), 1, 0);

    v3d::moya::MicroPolygon poly = grid.microPolygon(0, 0);

    BOOST_TEST((poly[0].point() == glm::vec3(0.0f, 0.0f, 0.0f)));
    BOOST_TEST((poly[1].point() == glm::vec3(0.0f, 1.0f, 0.0f)));
    BOOST_TEST((poly[2].point() == glm::vec3(1.0f, 1.0f, 0.0f)));
    BOOST_TEST((poly[3].point() == glm::vec3(1.0f, 0.0f, 0.0f)));
}

/**
 * A grid of n vertices a side holds n-1 micropolygons a side: the last row and column close
 * the grid rather than opening a polygon of their own.
 **/
BOOST_AUTO_TEST_CASE(micropolygon_grid_extent_test) {
    v3d::moya::MicroPolygonGrid grid(3);

    BOOST_TEST(grid.size() == 3u);
    // (1, 1) reads vertices (1, 1) through (2, 2), which is the last polygon that fits
    grid.addVertex(vertex(9.0f, 9.0f, 9.0f), 2, 2);
    BOOST_TEST((grid.microPolygon(1, 1)[2].point() == glm::vec3(9.0f, 9.0f, 9.0f)));
}

/**
 * A micropolygon turned 45 degrees covers the points inside it and not the corners of its
 * raster bound, which a hider filling the bound would have written.
 **/
BOOST_AUTO_TEST_CASE(micropolygon_covers_test) {
    const std::array<glm::vec3, 4> diamond = {
        glm::vec3(2.0f, 0.0f, 1.0f), glm::vec3(4.0f, 2.0f, 1.0f),
        glm::vec3(2.0f, 4.0f, 1.0f), glm::vec3(0.0f, 2.0f, 1.0f)
    };
    float depth = 0.0f;
    BOOST_CHECK(v3d::moya::covers(diamond, glm::vec2(2.0f, 2.0f), &depth));
    BOOST_CHECK_EQUAL(depth, 1.0f);
    BOOST_CHECK(v3d::moya::covers(diamond, glm::vec2(2.5f, 0.5f), &depth));
    BOOST_CHECK(v3d::moya::covers(diamond, glm::vec2(1.0f, 3.0f), &depth));
    // every corner of the bound is outside
    BOOST_CHECK(!v3d::moya::covers(diamond, glm::vec2(0.5f, 0.5f), &depth));
    BOOST_CHECK(!v3d::moya::covers(diamond, glm::vec2(3.5f, 0.5f), &depth));
    BOOST_CHECK(!v3d::moya::covers(diamond, glm::vec2(3.5f, 3.5f), &depth));
    BOOST_CHECK(!v3d::moya::covers(diamond, glm::vec2(0.5f, 3.5f), &depth));

    // the other winding is the same micropolygon
    const std::array<glm::vec3, 4> reversed = { diamond[3], diamond[2], diamond[1], diamond[0] };
    BOOST_CHECK(v3d::moya::covers(reversed, glm::vec2(2.0f, 2.0f), &depth));
    BOOST_CHECK(!v3d::moya::covers(reversed, glm::vec2(0.5f, 0.5f), &depth));

    // and one collapsed to a line covers nothing
    const std::array<glm::vec3, 4> line = {
        glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(4.0f, 0.0f, 1.0f),
        glm::vec3(4.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f)
    };
    BOOST_CHECK(!v3d::moya::covers(line, glm::vec2(2.0f, 0.0f), &depth));
}

/**
 * A depth is interpolated at the point rather than averaged over the corners. Two
 * micropolygons over the same square, tilted opposite ways, have the same mean depth, and
 * which is nearer depends on where in the square the sample is.
 **/
BOOST_AUTO_TEST_CASE(micropolygon_depth_is_interpolated_test) {
    const std::array<glm::vec3, 4> rising = {
        glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(4.0f, 0.0f, 3.0f),
        glm::vec3(4.0f, 4.0f, 3.0f), glm::vec3(0.0f, 4.0f, 1.0f)
    };
    const std::array<glm::vec3, 4> falling = {
        glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(4.0f, 0.0f, 1.0f),
        glm::vec3(4.0f, 4.0f, 1.0f), glm::vec3(0.0f, 4.0f, 3.0f)
    };
    float a = 0.0f;
    float b = 0.0f;
    BOOST_REQUIRE(v3d::moya::covers(rising, glm::vec2(1.0f, 2.0f), &a));
    BOOST_REQUIRE(v3d::moya::covers(falling, glm::vec2(1.0f, 2.0f), &b));
    BOOST_CHECK_CLOSE(a, 1.5f, 1.0e-4f);
    BOOST_CHECK_CLOSE(b, 2.5f, 1.0e-4f);

    BOOST_REQUIRE(v3d::moya::covers(rising, glm::vec2(3.0f, 2.0f), &a));
    BOOST_REQUIRE(v3d::moya::covers(falling, glm::vec2(3.0f, 2.0f), &b));
    BOOST_CHECK(b < a);
}

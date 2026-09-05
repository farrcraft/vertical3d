/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include <glm/glm.hpp>

#include "../libmoya/Polygon.h"

namespace {

    v3d::moya::Vertex vertex(float x, float y, float z) {
        v3d::moya::Vertex v;
        v.point(glm::vec3(x, y, z));
        return v;
    }

};  // namespace

BOOST_AUTO_TEST_CASE(polygon_vertex_test) {
    v3d::moya::Polygon polygon;

    BOOST_TEST(polygon.vertexCount() == 0u);

    glm::vec3 point(2.0f, 5.0f, 7.0f);
    polygon.addVertex(vertex(point.x, point.y, point.z));

    BOOST_TEST(polygon.vertexCount() == 1u);
    BOOST_TEST((polygon.vertex(0).point() == point));
    BOOST_TEST((polygon[0].point() == point));

    polygon.removeVertex(0);
    BOOST_TEST(polygon.vertexCount() == 0u);
}

/**
 * The subscript is the only accessor that hands back a reference, so it is the one a caller
 * can write a vertex through.
 **/
BOOST_AUTO_TEST_CASE(polygon_vertex_reference_test) {
    v3d::moya::Polygon polygon;
    polygon.addVertex(vertex(1.0f, 1.0f, 1.0f));

    polygon[0].point(glm::vec3(4.0f, 5.0f, 6.0f));

    BOOST_TEST((polygon.vertex(0).point() == glm::vec3(4.0f, 5.0f, 6.0f)));
}

BOOST_AUTO_TEST_CASE(polygon_remove_middle_vertex_test) {
    v3d::moya::Polygon polygon;
    polygon.addVertex(vertex(0.0f, 0.0f, 0.0f));
    polygon.addVertex(vertex(1.0f, 0.0f, 0.0f));
    polygon.addVertex(vertex(2.0f, 0.0f, 0.0f));

    polygon.removeVertex(1);

    BOOST_TEST(polygon.vertexCount() == 2u);
    BOOST_TEST((polygon.vertex(0).point() == glm::vec3(0.0f, 0.0f, 0.0f)));
    BOOST_TEST((polygon.vertex(1).point() == glm::vec3(2.0f, 0.0f, 0.0f)));
}

/**
 * clear empties the polygon, which is how a clip writes its result back over the one it was
 * given.
 **/
BOOST_AUTO_TEST_CASE(polygon_clear_test) {
    v3d::moya::Polygon polygon;
    polygon.addVertex(vertex(1.0f, 1.0f, 1.0f));
    polygon.addVertex(vertex(2.0f, 2.0f, 2.0f));

    polygon.clear();

    BOOST_TEST(polygon.vertexCount() == 0u);
}

/**
 * The bound is in object space and is the per-axis extent of the vertices, which is what the
 * splitter and the bucket assignment both read.
 **/
BOOST_AUTO_TEST_CASE(polygon_bound_test) {
    v3d::moya::Polygon polygon;
    polygon.addVertex(vertex(-1.0f, 4.0f, 0.0f));
    polygon.addVertex(vertex(3.0f, -2.0f, 5.0f));
    polygon.addVertex(vertex(0.0f, 1.0f, -7.0f));

    v3d::type::AABBox bound = polygon.bound();

    BOOST_TEST((bound.min() == glm::vec3(-1.0f, -2.0f, -7.0f)));
    BOOST_TEST((bound.max() == glm::vec3(3.0f, 4.0f, 5.0f)));
}

/**
 * A polygon holding nothing has no extent to report, so the bound comes back as the default
 * one rather than as whatever the first vertex would have seeded it with.
 **/
BOOST_AUTO_TEST_CASE(polygon_empty_bound_test) {
    v3d::moya::Polygon polygon;

    v3d::type::AABBox bound = polygon.bound();

    BOOST_TEST((bound.min() == glm::vec3(0.0f, 0.0f, 0.0f)));
    BOOST_TEST((bound.max() == glm::vec3(0.0f, 0.0f, 0.0f)));
}

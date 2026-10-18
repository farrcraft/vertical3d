/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/geometry/AABBox.h>
#include <moya/libmoya/Polygon.h>
#include <moya/libmoya/Vertex.h>

#include <boost/test/unit_test.hpp>

#include <glm/glm.hpp>

namespace {

v3d::moya::Vertex vertex(float x, float y, float z) {
    v3d::moya::Vertex v;
    v.point(glm::vec3(x, y, z));
    return v;
}

BOOST_AUTO_TEST_CASE(polygon_vertex_test) {
    v3d::moya::Polygon polygon;

    BOOST_TEST(polygon.vertexCount() == 0U);

    const glm::vec3 point(2.0F, 5.0F, 7.0F);
    polygon.addVertex(vertex(point.x, point.y, point.z));

    BOOST_TEST(polygon.vertexCount() == 1U);
    BOOST_TEST((polygon.vertex(0).point() == point));
    BOOST_TEST((polygon[0].point() == point));

    polygon.removeVertex(0);
    BOOST_TEST(polygon.vertexCount() == 0U);
}

/**
 * The subscript is the only accessor that returns a reference. A caller writes a vertex
 * through it.
 **/
BOOST_AUTO_TEST_CASE(polygon_vertex_reference_test) {
    v3d::moya::Polygon polygon;
    polygon.addVertex(vertex(1.0F, 1.0F, 1.0F));

    polygon[0].point(glm::vec3(4.0F, 5.0F, 6.0F));

    BOOST_TEST((polygon.vertex(0).point() == glm::vec3(4.0F, 5.0F, 6.0F)));
}

BOOST_AUTO_TEST_CASE(polygon_remove_middle_vertex_test) {
    v3d::moya::Polygon polygon;
    polygon.addVertex(vertex(0.0F, 0.0F, 0.0F));
    polygon.addVertex(vertex(1.0F, 0.0F, 0.0F));
    polygon.addVertex(vertex(2.0F, 0.0F, 0.0F));

    polygon.removeVertex(1);

    BOOST_TEST(polygon.vertexCount() == 2U);
    BOOST_TEST((polygon.vertex(0).point() == glm::vec3(0.0F, 0.0F, 0.0F)));
    BOOST_TEST((polygon.vertex(1).point() == glm::vec3(2.0F, 0.0F, 0.0F)));
}

/**
 * clear removes every vertex of the polygon.
 **/
BOOST_AUTO_TEST_CASE(polygon_clear_test) {
    v3d::moya::Polygon polygon;
    polygon.addVertex(vertex(1.0F, 1.0F, 1.0F));
    polygon.addVertex(vertex(2.0F, 2.0F, 2.0F));

    polygon.clear();

    BOOST_TEST(polygon.vertexCount() == 0U);
}

/**
 * The bound is the per-axis extent of the vertices, in the space they are in. split() and
 * RenderContext::addPolygon() both read it.
 **/
BOOST_AUTO_TEST_CASE(polygon_bound_test) {
    v3d::moya::Polygon polygon;
    polygon.addVertex(vertex(-1.0F, 4.0F, 0.0F));
    polygon.addVertex(vertex(3.0F, -2.0F, 5.0F));
    polygon.addVertex(vertex(0.0F, 1.0F, -7.0F));

    const v3d::type::geometry::AABBox bound = polygon.bound();

    BOOST_TEST((bound.min() == glm::vec3(-1.0F, -2.0F, -7.0F)));
    BOOST_TEST((bound.max() == glm::vec3(3.0F, 4.0F, 5.0F)));
}

/**
 * A polygon with no vertices has no extent, and its bound is a default constructed one.
 **/
BOOST_AUTO_TEST_CASE(polygon_empty_bound_test) {
    const v3d::moya::Polygon polygon;

    const v3d::type::geometry::AABBox bound = polygon.bound();

    BOOST_TEST((bound.min() == glm::vec3(0.0F, 0.0F, 0.0F)));
    BOOST_TEST((bound.max() == glm::vec3(0.0F, 0.0F, 0.0F)));
}

/**
 * The geometric normal is the normal of the plane the polygon lies in, facing the way its
 * vertices wind. It is SL's Ng. Every vertex takes it as its normal when the scene supplies no
 * varying "N".
 **/
BOOST_AUTO_TEST_CASE(polygon_geometric_normal_test) {
    v3d::moya::Polygon polygon;
    polygon.addVertex(vertex(0.0F, 0.0F, 0.0F));
    polygon.addVertex(vertex(1.0F, 0.0F, 0.0F));
    polygon.addVertex(vertex(0.0F, 1.0F, 0.0F));

    BOOST_TEST((polygon.geometricNormal() == glm::vec3(0.0F, 0.0F, 1.0F)));

    // the winding decides which way the polygon faces, so reversing it reverses the normal
    v3d::moya::Polygon reversed;
    reversed.addVertex(vertex(0.0F, 0.0F, 0.0F));
    reversed.addVertex(vertex(0.0F, 1.0F, 0.0F));
    reversed.addVertex(vertex(1.0F, 0.0F, 0.0F));

    BOOST_TEST((reversed.geometricNormal() == glm::vec3(0.0F, 0.0F, -1.0F)));
}

/**
 * The first three vertices need not be the three that name the plane. A repeated vertex or a
 * collinear run at the head of the polygon spans no area, and the search continues past it.
 **/
BOOST_AUTO_TEST_CASE(polygon_geometric_normal_skips_collinear_test) {
    v3d::moya::Polygon polygon;
    polygon.addVertex(vertex(0.0F, 0.0F, 0.0F));
    polygon.addVertex(vertex(1.0F, 0.0F, 0.0F));
    polygon.addVertex(vertex(2.0F, 0.0F, 0.0F));
    polygon.addVertex(vertex(0.0F, 1.0F, 0.0F));

    BOOST_TEST((polygon.geometricNormal() == glm::vec3(0.0F, 0.0F, 1.0F)));
}

/**
 * A polygon with no area lies in no plane, and its geometric normal is the zero vector.
 **/
BOOST_AUTO_TEST_CASE(polygon_degenerate_normal_test) {
    v3d::moya::Polygon collinear;
    collinear.addVertex(vertex(0.0F, 0.0F, 0.0F));
    collinear.addVertex(vertex(1.0F, 0.0F, 0.0F));
    collinear.addVertex(vertex(2.0F, 0.0F, 0.0F));

    BOOST_TEST((collinear.geometricNormal() == glm::vec3(0.0F)));

    const v3d::moya::Polygon empty;

    BOOST_TEST((empty.geometricNormal() == glm::vec3(0.0F)));
}

};  // namespace

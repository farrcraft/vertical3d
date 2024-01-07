/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../libmoya/Polygon.h"

BOOST_AUTO_TEST_CASE(polygon_test) {
    v3D::Moya::Polygon polygon;

    // polygon starts out with no vertices
    unsigned int count = 0;
    count = polygon.vertexCount();
    BOOST_CHECK_EQUAL(count, 0);

    // create a new polygon vertex
    v3D::Vector3 point(2.0f, 5.0f, 7.0f);
    v3D::Moya::Vertex vertex;
    vertex.point(point);
    polygon.addVertex(vertex);

    // should have 1 vertex now
    count = polygon.vertexCount();
    BOOST_CHECK_EQUAL(count, 1);

    // vertex we added should be the one we got back
    v3D::Moya::Vertex vertex2;
    vertex2 = polygon.vertex(0);
    // can only compare vertex points
    v3D::Vector3 point2;
    point2 = vertex2.point();
    BOOST_CHECK_EQUAL((point == point2), true);

    // alternate access method
    v3D::Moya::Vertex vertex3;
    vertex3 = polygon[0];
    v3D::Vector3 point3;
    point3 = vertex3.point();
    BOOST_CHECK_EQUAL((point == point3), true);

    // make sure removing vertices works
    polygon.removeVertex(0);
    count = polygon.vertexCount();
    BOOST_CHECK_EQUAL(count, 0);
}

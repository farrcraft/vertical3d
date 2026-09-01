/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../Vertex.h"

BOOST_AUTO_TEST_CASE(vertex_test) {
    v3d::brep::Vertex vertex;
    glm::vec3 vector(1.0f, 2.0f, 3.0f);
    glm::vec3 vector2(7.0f, 9.0f, 4.0f);
    vertex.point(vector);
    glm::vec3 checkVector = vertex.point();
    BOOST_CHECK_EQUAL((checkVector == vector), true);
    BOOST_CHECK_EQUAL((vertex == vector), true);
    BOOST_CHECK_EQUAL((vertex == vector2), false);

    v3d::brep::Vertex vertex2(vector);
    glm::vec3 checkPoint = vertex2.point();
    BOOST_CHECK_EQUAL((checkPoint == vector), true);
    BOOST_CHECK_EQUAL((vertex == vertex2), true);
    v3d::brep::Vertex vertex3(vector2);
    BOOST_CHECK_EQUAL((vertex2 == vertex3), false);

    vertex3.edge(11);
    BOOST_CHECK_EQUAL(vertex3.edge(), 11u);
}

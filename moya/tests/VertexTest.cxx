/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include <glm/glm.hpp>

#include "../libmoya/Vertex.h"

BOOST_AUTO_TEST_CASE(vertex_test) {
    v3d::moya::Vertex vertex;

    glm::vec3 point(7.0f, 12.0f, -13.0f);
    vertex.point(point);

    BOOST_TEST((vertex.point() == point));
}

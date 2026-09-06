/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../Face.h"

BOOST_AUTO_TEST_CASE(face_test) {
    v3d::brep::Face face;

    glm::vec3 normal(2.0f, 9.0f, 42.0f);
    face.normal(normal);
    glm::vec3 checkNormal = face.normal();
    BOOST_CHECK_EQUAL((normal == checkNormal), true);

    face.edge(7);
    BOOST_CHECK_EQUAL(face.edge(), 7u);

    v3d::brep::Face face2(normal, 11);
    glm::vec3 checkNormal2 = face2.normal();
    BOOST_CHECK_EQUAL((normal == checkNormal2), true);
    BOOST_CHECK_EQUAL(face2.edge(), 11u);
}

BOOST_AUTO_TEST_CASE(face_selection_test) {
    v3d::brep::Face face;

    BOOST_CHECK_EQUAL(face.selected(), false);
    face.selected(true);
    BOOST_CHECK_EQUAL(face.selected(), true);
    face.selected(false);
    BOOST_CHECK_EQUAL(face.selected(), false);
}

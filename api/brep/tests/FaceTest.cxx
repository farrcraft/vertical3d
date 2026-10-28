/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/Face.h>

#include <boost/test/unit_test.hpp>

namespace {

BOOST_AUTO_TEST_CASE(face_test) {
    v3d::brep::Face face;

    const glm::vec3 normal(2.0F, 9.0F, 42.0F);
    face.normal(normal);
    const glm::vec3 checkNormal = face.normal();
    BOOST_CHECK_EQUAL((normal == checkNormal), true);

    face.edge(7);
    BOOST_CHECK_EQUAL(face.edge(), 7U);

    const v3d::brep::Face face2(normal, 11);
    const glm::vec3 checkNormal2 = face2.normal();
    BOOST_CHECK_EQUAL((normal == checkNormal2), true);
    BOOST_CHECK_EQUAL(face2.edge(), 11U);
}

};  // namespace

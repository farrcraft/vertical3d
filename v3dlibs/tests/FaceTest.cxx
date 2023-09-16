#include <boost/test/unit_test.hpp>

#include "../brep/Face.h"

BOOST_AUTO_TEST_CASE( face_test )
{
    v3D::Face face;

    v3D::Vector3 normal(2.0f, 9.0f, 42.0f);
    face.normal(normal);
    v3D::Vector3 checkNormal = face.normal();
    BOOST_CHECK_EQUAL((normal == checkNormal), true);

    face.edge(7);
    BOOST_CHECK_EQUAL((face.edge() == 7), true);

    v3D::Face face2(normal, 11);
    v3D::Vector3 checkNormal2 = face2.normal();
    BOOST_CHECK_EQUAL((normal == checkNormal2), true);
    BOOST_CHECK_EQUAL((face2.edge() == 11), true);
}
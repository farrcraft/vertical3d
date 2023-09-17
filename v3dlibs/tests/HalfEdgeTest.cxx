#include <boost/test/unit_test.hpp>

#include "../brep/HalfEdge.h"

BOOST_AUTO_TEST_CASE( halfEdge_test )
{
    v3D::HalfEdge edge;

    edge.face(0);
    BOOST_CHECK_EQUAL(edge.face(), 0);

    edge.next(1);
    BOOST_CHECK_EQUAL(edge.next(), 1);

    edge.pair(2);
    BOOST_CHECK_EQUAL(edge.pair(), 2);

    edge.vertex(3);
    BOOST_CHECK_EQUAL(edge.vertex(), 3);

    v3D::HalfEdge edge2(4);
    BOOST_CHECK_EQUAL(edge2.vertex(), 4);

    BOOST_CHECK_EQUAL((edge == edge2), false);

    v3D::HalfEdge edge3(edge);
    BOOST_CHECK_EQUAL((edge3 == edge), true);

    edge2 = edge;
    BOOST_CHECK_EQUAL((edge2 == edge), true);
}
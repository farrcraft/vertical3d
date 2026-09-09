/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/HalfEdge.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(halfEdge_test) {
    v3d::brep::HalfEdge edge;

    edge.face(0);
    BOOST_CHECK_EQUAL(edge.face(), 0u);

    edge.next(1);
    BOOST_CHECK_EQUAL(edge.next(), 1u);

    edge.pair(2);
    BOOST_CHECK_EQUAL(edge.pair(), 2u);

    edge.vertex(3);
    BOOST_CHECK_EQUAL(edge.vertex(), 3u);

    v3d::brep::HalfEdge edge2(4);
    BOOST_CHECK_EQUAL(edge2.vertex(), 4u);

    BOOST_CHECK_EQUAL((edge == edge2), false);

    v3d::brep::HalfEdge edge3(edge);
    BOOST_CHECK_EQUAL((edge3 == edge), true);

    edge2 = edge;
    BOOST_CHECK_EQUAL((edge2 == edge), true);
}

BOOST_AUTO_TEST_CASE(halfedge_selection_test) {
    v3d::brep::HalfEdge edge(3);

    BOOST_CHECK_EQUAL(edge.selected(), false);
    edge.selected(true);
    BOOST_CHECK_EQUAL(edge.selected(), true);

    // a copy carries the selection, since a BRep stores edges by value and copies them
    v3d::brep::HalfEdge duplicate(edge);
    BOOST_CHECK_EQUAL(duplicate.selected(), true);

    // equality is topological, so it ignores the flag
    v3d::brep::HalfEdge other(3);
    BOOST_CHECK_EQUAL((edge == other), true);
}

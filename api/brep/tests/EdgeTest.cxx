/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <algorithm>

#include <boost/test/unit_test.hpp>

#include "../Edge.h"

/**
 * A winged edge names eight things: a vertex at each end, a face on each side, and the four
 * edges it meets at those ends.
 **/
BOOST_AUTO_TEST_CASE(edge_test) {
    v3d::brep::Edge edge;

    edge.prevVertex(0);
    edge.nextVertex(1);
    edge.prevFace(2);
    edge.nextFace(3);
    edge.prevCWEdge(4);
    edge.nextCWEdge(5);
    edge.prevCCWEdge(6);
    edge.nextCCWEdge(7);

    BOOST_CHECK_EQUAL(edge.prevVertex(), 0u);
    BOOST_CHECK_EQUAL(edge.nextVertex(), 1u);
    BOOST_CHECK_EQUAL(edge.prevFace(), 2u);
    BOOST_CHECK_EQUAL(edge.nextFace(), 3u);
    BOOST_CHECK_EQUAL(edge.prevCWEdge(), 4u);
    BOOST_CHECK_EQUAL(edge.nextCWEdge(), 5u);
    BOOST_CHECK_EQUAL(edge.prevCCWEdge(), 6u);
    BOOST_CHECK_EQUAL(edge.nextCCWEdge(), 7u);
}

/**
 * Every reference starts absent. A default edge whose members held whatever was on the stack
 * would be indistinguishable from one pointing at real geometry.
 **/
BOOST_AUTO_TEST_CASE(edge_default_is_unset_test) {
    v3d::brep::Edge edge;

    BOOST_CHECK_EQUAL(edge.prevVertex(), v3d::brep::INVALID_ID);
    BOOST_CHECK_EQUAL(edge.nextVertex(), v3d::brep::INVALID_ID);
    BOOST_CHECK_EQUAL(edge.prevFace(), v3d::brep::INVALID_ID);
    BOOST_CHECK_EQUAL(edge.nextFace(), v3d::brep::INVALID_ID);
    BOOST_CHECK_EQUAL(edge.prevCWEdge(), v3d::brep::INVALID_ID);
    BOOST_CHECK_EQUAL(edge.nextCWEdge(), v3d::brep::INVALID_ID);
    BOOST_CHECK_EQUAL(edge.prevCCWEdge(), v3d::brep::INVALID_ID);
    BOOST_CHECK_EQUAL(edge.nextCCWEdge(), v3d::brep::INVALID_ID);
    BOOST_CHECK_EQUAL(edge.selected(), false);
}

/**
 * The two-vertex constructor sets the ends and leaves everything else absent, so an edge
 * added before its faces and wings are known reads as incomplete rather than as wrong.
 **/
BOOST_AUTO_TEST_CASE(edge_endpoints_constructor_test) {
    v3d::brep::Edge edge(3, 4);

    BOOST_CHECK_EQUAL(edge.prevVertex(), 3u);
    BOOST_CHECK_EQUAL(edge.nextVertex(), 4u);
    BOOST_CHECK_EQUAL(edge.prevFace(), v3d::brep::INVALID_ID);
    BOOST_CHECK_EQUAL(edge.nextCCWEdge(), v3d::brep::INVALID_ID);
    BOOST_CHECK_EQUAL(edge.selected(), false);
}

/**
 * An edge is unordered: it is the same edge whichever end is called first, which is what
 * lets addEdge find one that is already there.
 **/
BOOST_AUTO_TEST_CASE(edge_equality_ignores_direction_test) {
    const v3d::brep::Edge edge(3, 4);
    const v3d::brep::Edge same(3, 4);
    const v3d::brep::Edge reversed(4, 3);
    const v3d::brep::Edge other(3, 5);

    BOOST_CHECK_EQUAL((edge == same), true);
    BOOST_CHECK_EQUAL((edge == reversed), true);
    BOOST_CHECK_EQUAL((edge == other), false);
}

BOOST_AUTO_TEST_CASE(edge_copy_test) {
    v3d::brep::Edge edge(3, 4);
    edge.prevFace(5);
    edge.selected(true);

    const v3d::brep::Edge copy(edge);
    BOOST_CHECK_EQUAL(copy.prevVertex(), 3u);
    BOOST_CHECK_EQUAL(copy.prevFace(), 5u);
    BOOST_CHECK_EQUAL(copy.selected(), true);

    v3d::brep::Edge assigned;
    assigned = edge;
    BOOST_CHECK_EQUAL(assigned.nextVertex(), 4u);
    BOOST_CHECK_EQUAL(assigned.prevFace(), 5u);
    BOOST_CHECK_EQUAL(assigned.selected(), true);
}

/**
 * Selection is per component, so an edge carries its own flag.
 **/
BOOST_AUTO_TEST_CASE(edge_selection_test) {
    v3d::brep::Edge edge(3, 4);

    BOOST_CHECK_EQUAL(edge.selected(), false);
    edge.selected(true);
    BOOST_CHECK_EQUAL(edge.selected(), true);
    edge.selected(false);
    BOOST_CHECK_EQUAL(edge.selected(), false);
}

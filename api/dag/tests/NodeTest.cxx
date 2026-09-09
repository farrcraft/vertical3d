/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/dag/Node.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(node_id_test) {
    v3d::dag::Node first;
    v3d::dag::Node second;

    // the selection model keys on the id, so two nodes never share one
    BOOST_CHECK(first.id() != second.id());
    BOOST_CHECK(second.id() > first.id());
    BOOST_CHECK(v3d::dag::Node::baseID() >= second.id());
}

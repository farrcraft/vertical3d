/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/dag/Node.h>

#include <boost/test/unit_test.hpp>

namespace {

BOOST_AUTO_TEST_CASE(node_id_test) {
    const v3d::dag::Node first;
    const v3d::dag::Node second;

    // two nodes never share an id, and a later node has a larger one
    BOOST_CHECK(first.id() != second.id());
    BOOST_CHECK(second.id() > first.id());
    BOOST_CHECK(v3d::dag::Node::baseID() >= second.id());
}

};  // namespace

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Index.h>
#include <api/brep/Primitives.h>
#include <vertical3d/src/scene/Components.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>

#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

BOOST_AUTO_TEST_CASE(components_select_test) {
    v3d::editor::Components components;
    BOOST_CHECK(!components.any(1));

    // each kind of part is kept apart, and comes back rising whatever order it was picked in
    components.select(1, v3d::editor::Component::Face, 5, true);
    components.select(1, v3d::editor::Component::Face, 2, true);
    components.select(1, v3d::editor::Component::Vertex, 2, true);
    BOOST_CHECK(components.selected(1, v3d::editor::Component::Face, 2));
    BOOST_CHECK(!components.selected(1, v3d::editor::Component::Edge, 2));
    BOOST_CHECK((components.of(1, v3d::editor::Component::Face) == std::vector<v3d::brep::Index>{ 2, 5 }));
    BOOST_CHECK(components.of(2, v3d::editor::Component::Face).empty());

    // a part deselected goes, and an item with nothing left selected has nothing
    components.select(1, v3d::editor::Component::Face, 5, false);
    components.select(1, v3d::editor::Component::Face, 2, false);
    BOOST_CHECK(components.any(1));
    components.select(1, v3d::editor::Component::Vertex, 2, false);
    BOOST_CHECK(!components.any(1));

    // clearing one item leaves the others
    components.select(1, v3d::editor::Component::Vertex, 0, true);
    components.select(2, v3d::editor::Component::Vertex, 0, true);
    components.clear(1);
    BOOST_CHECK(!components.any(1));
    BOOST_CHECK(components.any(2));
    components.clear();
    BOOST_CHECK(!components.any(2));
}

BOOST_AUTO_TEST_CASE(components_edge_test) {
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::brep::cube();
    v3d::brep::Index paired = 0;
    while (cube->edge(paired)->pair() == v3d::brep::INVALID_ID) {
        paired++;
    }
    const v3d::brep::Index other = cube->edge(paired)->pair();

    // an edge is selected when either of its halves is
    v3d::editor::Components components;
    components.select(1, v3d::editor::Component::Edge, other, true);
    BOOST_CHECK(components.edge(1, *cube, paired));
    BOOST_CHECK(components.edge(1, *cube, other));
    BOOST_CHECK(!components.selected(1, v3d::editor::Component::Edge, paired));
    BOOST_CHECK(!components.edge(2, *cube, paired));
}

/**
 * The selection belongs to the item, so two items showing one mesh select its parts apart.
 **/
BOOST_AUTO_TEST_CASE(components_shared_mesh_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::brep::BRep> shared = v3d::brep::cube();
    const boost::shared_ptr<v3d::editor::Mesh> first = boost::make_shared<v3d::editor::Mesh>(shared);
    const boost::shared_ptr<v3d::editor::Mesh> second = boost::make_shared<v3d::editor::Mesh>(shared);
    scene.add(first);
    scene.add(second);

    scene.components().select(first->id(), v3d::editor::Component::Face, 0, true);
    BOOST_CHECK(scene.components().selected(first->id(), v3d::editor::Component::Face, 0));
    BOOST_CHECK(!scene.components().any(second->id()));

    // a change to the mesh's numbering clears every item that shows it
    scene.components().select(second->id(), v3d::editor::Component::Vertex, 1, true);
    scene.deselectComponents(shared.get());
    BOOST_CHECK(!scene.components().any(first->id()));
    BOOST_CHECK(!scene.components().any(second->id()));

    // and a removed item takes its parts with it
    scene.components().select(second->id(), v3d::editor::Component::Vertex, 1, true);
    BOOST_REQUIRE(scene.remove(second->id()));
    BOOST_CHECK(!scene.components().any(second->id()));
}

};  // namespace

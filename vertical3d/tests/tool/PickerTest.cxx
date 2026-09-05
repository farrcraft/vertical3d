/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/geometric.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "../../src/scene/CreatePoly.h"
#include "../../src/scene/MeshTopology.h"
#include "../../src/tool/Picker.h"
#include "../../src/scene/Scene.h"
#include "../../src/view/ViewPort.h"

namespace {

/**
 * A square front view, four hundred pixels on a side, of the default orthographic
 * profile: the eye is at z of -1 looking along +z, and the volume spans [-1, 1] on both
 * axes - so the unit primitives cover the middle half of it and a world unit is two
 * hundred pixels.
 **/
boost::shared_ptr<v3d::editor::ViewPort> frontView() {
    v3d::type::CameraProfile profile("front");
    boost::shared_ptr<v3d::editor::ViewPort> view = boost::make_shared<v3d::editor::ViewPort>("front", profile);
    view->resize(glm::vec4(0.0f, 0.0f, 400.0f, 400.0f));
    return view;
}

/**
 * Where a world point lands in that view, which is how a test aims a click at a
 * particular piece of geometry.
 **/
glm::vec2 screen(const boost::shared_ptr<v3d::editor::ViewPort>& view, const glm::vec3& point) {
    boost::shared_ptr<v3d::type::Camera> camera = view->camera();
    camera->createProjection();
    camera->createView();
    int viewport[4] = { 0, 0, 400, 400 };
    const glm::vec3 projected = camera->project(point, viewport);
    return glm::vec2(projected[0], projected[1]);
}

};  // namespace

BOOST_AUTO_TEST_CASE(picker_object_test) {
    v3d::editor::Scene scene;
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    const unsigned int id = scene.add(cube);

    boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;

    // the middle of the view is the middle of the cube
    v3d::editor::Picker::Hit hit = picker.pick(scene, *view, glm::vec2(200.0f, 200.0f),
        v3d::editor::SelectMask::Object);
    BOOST_CHECK_EQUAL(hit.valid, true);
    BOOST_CHECK_EQUAL(hit.mesh, id);
    BOOST_CHECK(hit.kind == v3d::editor::SelectMask::Object);

    // the corner of the view is well outside a mesh that covers the middle half of it
    v3d::editor::Picker::Hit missed = picker.pick(scene, *view, glm::vec2(10.0f, 10.0f),
        v3d::editor::SelectMask::Object);
    BOOST_CHECK_EQUAL(missed.valid, false);
}

BOOST_AUTO_TEST_CASE(picker_nearest_test) {
    v3d::editor::Scene scene;

    // not near/far: the windows headers define both as macros
    boost::shared_ptr<v3d::brep::BRep> behind = v3d::editor::create_poly_cube();
    behind->translation(glm::vec3(0.0f, 0.0f, 4.0f));
    scene.add(behind);

    boost::shared_ptr<v3d::brep::BRep> front = v3d::editor::create_poly_cube();
    front->translation(glm::vec3(0.0f, 0.0f, 2.0f));
    const unsigned int id = scene.add(front);

    // both are under the cursor, and the one nearer the camera along the ray wins - the
    // second added, so insertion order is not what decides it
    boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;
    v3d::editor::Picker::Hit hit = picker.pick(scene, *view, glm::vec2(200.0f, 200.0f),
        v3d::editor::SelectMask::Object);
    BOOST_CHECK_EQUAL(hit.valid, true);
    BOOST_CHECK_EQUAL(hit.mesh, id);
}

BOOST_AUTO_TEST_CASE(picker_transform_test) {
    v3d::editor::Scene scene;
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    cube->translation(glm::vec3(0.75f, 0.0f, 0.0f));
    const unsigned int id = scene.add(cube);

    boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;

    // the mesh is picked where it is drawn, which is through its transform - the middle of
    // the view is now beside it
    v3d::editor::Picker::Hit centre = picker.pick(scene, *view, glm::vec2(200.0f, 200.0f),
        v3d::editor::SelectMask::Object);
    BOOST_CHECK_EQUAL(centre.valid, false);

    v3d::editor::Picker::Hit moved = picker.pick(scene, *view, screen(view, glm::vec3(0.75f, 0.0f, 0.0f)),
        v3d::editor::SelectMask::Object);
    BOOST_CHECK_EQUAL(moved.valid, true);
    BOOST_CHECK_EQUAL(moved.mesh, id);
}

BOOST_AUTO_TEST_CASE(picker_face_test) {
    v3d::editor::Scene scene;
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    cube->selected(true);
    const unsigned int id = scene.add(cube);

    boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;

    v3d::editor::Picker::Hit hit = picker.pick(scene, *view, glm::vec2(200.0f, 200.0f),
        v3d::editor::SelectMask::Face);
    BOOST_CHECK_EQUAL(hit.valid, true);
    BOOST_CHECK_EQUAL(hit.mesh, id);
    BOOST_CHECK(hit.kind == v3d::editor::SelectMask::Face);
    BOOST_CHECK_LT(hit.component, cube->faceCount());

    // the face hit is the one nearest the camera, so the ray enters through it
    const std::vector<unsigned int> loop = v3d::editor::faceLoop(cube, hit.component);
    BOOST_CHECK_GE(loop.size(), 3u);
    glm::vec3 from;
    BOOST_CHECK_EQUAL(v3d::editor::loopSegment(cube, loop, 0, &from, nullptr), true);
    BOOST_CHECK_LT(from[2], 0.0f);
}

BOOST_AUTO_TEST_CASE(picker_component_needs_an_object_test) {
    v3d::editor::Scene scene;
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    scene.add(cube);

    boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;

    // an object has to be selected before any of its components may be
    v3d::editor::Picker::Hit ignored = picker.pick(scene, *view, glm::vec2(200.0f, 200.0f),
        v3d::editor::SelectMask::Face);
    BOOST_CHECK_EQUAL(ignored.valid, false);

    cube->selected(true);
    v3d::editor::Picker::Hit hit = picker.pick(scene, *view, glm::vec2(200.0f, 200.0f),
        v3d::editor::SelectMask::Face);
    BOOST_CHECK_EQUAL(hit.valid, true);
}

BOOST_AUTO_TEST_CASE(picker_vertex_test) {
    v3d::editor::Scene scene;
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    cube->selected(true);
    scene.add(cube);

    boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;

    v3d::brep::Vertex* first = cube->vertex(0);
    BOOST_REQUIRE(first != nullptr);
    const glm::vec2 target = screen(view, first->point());

    v3d::editor::Picker::Hit hit = picker.pick(scene, *view, target, v3d::editor::SelectMask::Vertex);
    BOOST_CHECK_EQUAL(hit.valid, true);
    BOOST_CHECK(hit.kind == v3d::editor::SelectMask::Vertex);

    // a front view of a cube projects its front and back vertices onto the same point, so
    // which index answers is decided by depth - whichever it is has to be under the cursor
    v3d::brep::Vertex* found = cube->vertex(hit.component);
    BOOST_REQUIRE(found != nullptr);
    BOOST_CHECK_LT(glm::distance(screen(view, found->point()), target), 5.0f);

    // the middle of a face is a hundred pixels from the nearest vertex, which is well
    // outside the tolerance
    v3d::editor::Picker::Hit missed = picker.pick(scene, *view, glm::vec2(200.0f, 200.0f),
        v3d::editor::SelectMask::Vertex);
    BOOST_CHECK_EQUAL(missed.valid, false);
}

BOOST_AUTO_TEST_CASE(picker_edge_test) {
    v3d::editor::Scene scene;
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    cube->selected(true);
    scene.add(cube);

    boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;

    const std::vector<unsigned int> loop = v3d::editor::faceLoop(cube, 0);
    BOOST_REQUIRE_GE(loop.size(), 3u);
    glm::vec3 from, to;
    BOOST_REQUIRE_EQUAL(v3d::editor::loopSegment(cube, loop, 0, &from, &to), true);

    // the middle of an edge, which is the point furthest from either of its vertices
    v3d::editor::Picker::Hit hit = picker.pick(scene, *view, screen(view, (from + to) * 0.5f),
        v3d::editor::SelectMask::Edge);
    BOOST_CHECK_EQUAL(hit.valid, true);
    BOOST_CHECK(hit.kind == v3d::editor::SelectMask::Edge);
    BOOST_CHECK_LT(hit.component, cube->edgeCount());

    // the middle of a face is a hundred pixels from any of them
    v3d::editor::Picker::Hit missed = picker.pick(scene, *view, glm::vec2(200.0f, 200.0f),
        v3d::editor::SelectMask::Edge);
    BOOST_CHECK_EQUAL(missed.valid, false);
}

BOOST_AUTO_TEST_CASE(picker_empty_test) {
    v3d::editor::Scene scene;
    v3d::editor::Picker picker;

    boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    BOOST_CHECK_EQUAL(picker.pick(scene, *view, glm::vec2(200.0f, 200.0f),
        v3d::editor::SelectMask::Object).valid, false);

    // a view that has never been given a region divides by zero if it is picked in
    v3d::type::CameraProfile profile("unsized");
    v3d::editor::ViewPort unsized("unsized", profile);
    scene.add(v3d::editor::create_poly_cube());
    BOOST_CHECK_EQUAL(picker.pick(scene, unsized, glm::vec2(0.0f, 0.0f),
        v3d::editor::SelectMask::Object).valid, false);
}

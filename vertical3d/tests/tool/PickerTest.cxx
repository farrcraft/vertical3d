/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/Index.h>
#include <api/brep/Topology.h>
#include <api/brep/Vertex.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/CreatePoly.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/tool/Picker.h>
#include <vertical3d/src/tool/SelectMask.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <optional>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/geometric.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

/**
 * A square front view, four hundred pixels on a side, of the default orthographic
 * profile. The eye is at z of -1 looking along +z, and the volume spans [-1, 1] on both
 * axes. The unit primitives therefore cover the middle half of it, and a world unit is two
 * hundred pixels.
 **/
boost::shared_ptr<v3d::editor::ViewPort> frontView() {
    const v3d::type::camera::Profile profile("front");
    boost::shared_ptr<v3d::editor::ViewPort> view = boost::make_shared<v3d::editor::ViewPort>("front", profile);
    view->resize(glm::vec4(0.0F, 0.0F, 400.0F, 400.0F));
    return view;
}

/**
 * Where a world point lands in that view. A test uses it to aim a click at a particular
 * piece of geometry.
 **/
glm::vec2 screen(const boost::shared_ptr<v3d::editor::ViewPort>& view, const glm::vec3& point) {
    const boost::shared_ptr<v3d::type::camera::Camera> camera = view->camera();
    camera->createProjection();
    camera->createView();
    int viewport[4] = { 0, 0, 400, 400 };
    const glm::vec3 projected = camera->project(point, viewport);
    return glm::vec2(projected[0], projected[1]);
}

BOOST_AUTO_TEST_CASE(picker_object_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    const unsigned int id = scene.add(cube);

    const boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;

    // the middle of the view is the middle of the cube
    const v3d::editor::Picker::Hit hit = picker.pick(scene, *view, glm::vec2(200.0F, 200.0F),
        v3d::editor::SelectMask::Object);
    BOOST_CHECK_EQUAL(hit.valid, true);
    BOOST_CHECK_EQUAL(hit.mesh, id);
    BOOST_CHECK(hit.kind == v3d::editor::SelectMask::Object);

    // the corner of the view is well outside a mesh that covers the middle half of it
    const v3d::editor::Picker::Hit missed = picker.pick(scene, *view, glm::vec2(10.0F, 10.0F),
        v3d::editor::SelectMask::Object);
    BOOST_CHECK_EQUAL(missed.valid, false);
}

BOOST_AUTO_TEST_CASE(picker_nearest_test) {
    v3d::editor::Scene scene;

    // not near/far: the windows headers define both as macros
    const boost::shared_ptr<v3d::editor::Mesh> behind = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    behind->translation(glm::vec3(0.0F, 0.0F, 4.0F));
    scene.add(behind);

    const boost::shared_ptr<v3d::editor::Mesh> front = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    front->translation(glm::vec3(0.0F, 0.0F, 2.0F));
    const unsigned int id = scene.add(front);

    // both are under the cursor, and the one nearer the camera along the ray wins. It is
    // the second added, so insertion order does not decide it
    const boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;
    const v3d::editor::Picker::Hit hit = picker.pick(scene, *view, glm::vec2(200.0F, 200.0F),
        v3d::editor::SelectMask::Object);
    BOOST_CHECK_EQUAL(hit.valid, true);
    BOOST_CHECK_EQUAL(hit.mesh, id);
}

BOOST_AUTO_TEST_CASE(picker_transform_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    cube->translation(glm::vec3(0.75F, 0.0F, 0.0F));
    const unsigned int id = scene.add(cube);

    const boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;

    // the mesh is picked where it is drawn, which is through its transform. The middle of
    // the view is now beside it
    const v3d::editor::Picker::Hit centre = picker.pick(scene, *view, glm::vec2(200.0F, 200.0F),
        v3d::editor::SelectMask::Object);
    BOOST_CHECK_EQUAL(centre.valid, false);

    const v3d::editor::Picker::Hit moved = picker.pick(scene, *view, screen(view, glm::vec3(0.75F, 0.0F, 0.0F)),
        v3d::editor::SelectMask::Object);
    BOOST_CHECK_EQUAL(moved.valid, true);
    BOOST_CHECK_EQUAL(moved.mesh, id);
}

BOOST_AUTO_TEST_CASE(picker_face_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    cube->selected(true);
    const unsigned int id = scene.add(cube);

    const boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;

    const v3d::editor::Picker::Hit hit = picker.pick(scene, *view, glm::vec2(200.0F, 200.0F),
        v3d::editor::SelectMask::Face);
    BOOST_CHECK_EQUAL(hit.valid, true);
    BOOST_CHECK_EQUAL(hit.mesh, id);
    BOOST_CHECK(hit.kind == v3d::editor::SelectMask::Face);
    BOOST_CHECK_LT(hit.component, cube->geometry()->faceCount());

    // the face hit is the one nearest the camera, so the ray enters through it
    const std::vector<v3d::brep::Index> loop = v3d::brep::faceLoop(*cube->geometry(), hit.component);
    BOOST_CHECK_GE(loop.size(), 3U);
    const std::optional<v3d::brep::Segment> found = v3d::brep::loopSegment(*cube->geometry(), loop, 0);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_LT(found.value_or(v3d::brep::Segment()).from[2], 0.0F);
}

BOOST_AUTO_TEST_CASE(picker_component_needs_an_object_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    scene.add(cube);

    const boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;

    // an object has to be selected before any of its components may be
    const v3d::editor::Picker::Hit ignored = picker.pick(scene, *view, glm::vec2(200.0F, 200.0F),
        v3d::editor::SelectMask::Face);
    BOOST_CHECK_EQUAL(ignored.valid, false);

    cube->selected(true);
    const v3d::editor::Picker::Hit hit = picker.pick(scene, *view, glm::vec2(200.0F, 200.0F),
        v3d::editor::SelectMask::Face);
    BOOST_CHECK_EQUAL(hit.valid, true);
}

BOOST_AUTO_TEST_CASE(picker_vertex_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    cube->selected(true);
    scene.add(cube);

    const boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;

    const v3d::brep::Vertex* first = cube->geometry()->vertex(0);
    BOOST_REQUIRE(first != nullptr);
    const glm::vec2 target = screen(view, first->point());

    const v3d::editor::Picker::Hit hit = picker.pick(scene, *view, target, v3d::editor::SelectMask::Vertex);
    BOOST_CHECK_EQUAL(hit.valid, true);
    BOOST_CHECK(hit.kind == v3d::editor::SelectMask::Vertex);

    // a front view of a cube projects its front and back vertices onto the same point, so
    // depth decides which index is returned. Whichever it is has to be under the cursor
    const v3d::brep::Vertex* found = cube->geometry()->vertex(hit.component);
    BOOST_REQUIRE(found != nullptr);
    BOOST_CHECK_LT(glm::distance(screen(view, found->point()), target), 5.0F);

    // the middle of a face is a hundred pixels from the nearest vertex, which is well
    // outside the tolerance
    const v3d::editor::Picker::Hit missed = picker.pick(scene, *view, glm::vec2(200.0F, 200.0F),
        v3d::editor::SelectMask::Vertex);
    BOOST_CHECK_EQUAL(missed.valid, false);
}

BOOST_AUTO_TEST_CASE(picker_edge_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    cube->selected(true);
    scene.add(cube);

    const boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;

    const std::vector<v3d::brep::Index> loop = v3d::brep::faceLoop(*cube->geometry(), 0);
    BOOST_REQUIRE_GE(loop.size(), 3U);
    const std::optional<v3d::brep::Segment> found = v3d::brep::loopSegment(*cube->geometry(), loop, 0);
    BOOST_REQUIRE(found.has_value());
    const v3d::brep::Segment segment = found.value_or(v3d::brep::Segment());
    const glm::vec3& from = segment.from;
    const glm::vec3& to = segment.to;

    // the middle of an edge, the point furthest from either of its vertices
    const v3d::editor::Picker::Hit hit = picker.pick(scene, *view, screen(view, (from + to) * 0.5F),
        v3d::editor::SelectMask::Edge);
    BOOST_CHECK_EQUAL(hit.valid, true);
    BOOST_CHECK(hit.kind == v3d::editor::SelectMask::Edge);
    BOOST_CHECK_LT(hit.component, cube->geometry()->edgeCount());

    // the middle of a face is a hundred pixels from any of them
    const v3d::editor::Picker::Hit missed = picker.pick(scene, *view, glm::vec2(200.0F, 200.0F),
        v3d::editor::SelectMask::Edge);
    BOOST_CHECK_EQUAL(missed.valid, false);
}

/**
 * How many things of a mask a rectangle between two world points holds, in the front view.
 **/
std::size_t inside(const v3d::editor::Scene& scene, const glm::vec3& corner, const glm::vec3& opposite,
    v3d::editor::SelectMask mask) {
    const boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;
    return picker.inside(scene, *view, screen(view, corner), screen(view, opposite), mask).size();
}

BOOST_AUTO_TEST_CASE(picker_inside_object_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    const unsigned int id = scene.add(cube);

    const boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;
    const std::vector<v3d::editor::Picker::Hit> hits = picker.inside(scene, *view,
        screen(view, glm::vec3(-0.6F, -0.6F, 0.0F)), screen(view, glm::vec3(0.6F, 0.6F, 0.0F)),
        v3d::editor::SelectMask::Object);
    BOOST_REQUIRE_EQUAL(hits.size(), 1U);
    BOOST_CHECK_EQUAL(hits[0].valid, true);
    BOOST_CHECK_EQUAL(hits[0].mesh, id);
    BOOST_CHECK(hits[0].kind == v3d::editor::SelectMask::Object);

    // the corners may be given either way round
    BOOST_CHECK_EQUAL(inside(scene, glm::vec3(0.6F, -0.6F, 0.0F), glm::vec3(-0.6F, 0.6F, 0.0F),
        v3d::editor::SelectMask::Object), 1U);

    // an object only partly inside is not inside
    BOOST_CHECK_EQUAL(inside(scene, glm::vec3(-0.6F, -0.6F, 0.0F), glm::vec3(0.4F, 0.6F, 0.0F),
        v3d::editor::SelectMask::Object), 0U);

    // the object's transform moves its bound
    cube->translation(glm::vec3(2.0F, 0.0F, 0.0F));
    BOOST_CHECK_EQUAL(inside(scene, glm::vec3(-0.6F, -0.6F, 0.0F), glm::vec3(0.6F, 0.6F, 0.0F),
        v3d::editor::SelectMask::Object), 0U);
}

BOOST_AUTO_TEST_CASE(picker_inside_component_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    scene.add(cube);

    const glm::vec3 bottomLeft(-0.6F, -0.6F, 0.0F);
    const glm::vec3 topRight(0.6F, 0.6F, 0.0F);
    // a strip down the cube's left side, which in the front view is its whole left face
    const glm::vec3 leftSide(-0.4F, 0.6F, 0.0F);
    // one corner of the cube, where a vertex at the front and one at the back both land
    const glm::vec3 oneVertex(-0.4F, -0.4F, 0.0F);

    // a component mask considers only selected objects
    BOOST_CHECK_EQUAL(inside(scene, bottomLeft, topRight, v3d::editor::SelectMask::Vertex), 0U);
    cube->selected(true);

    // hidden parts count, because a band tests geometry and not pixels
    BOOST_CHECK_EQUAL(inside(scene, bottomLeft, topRight, v3d::editor::SelectMask::Vertex), 8U);
    BOOST_CHECK_EQUAL(inside(scene, bottomLeft, oneVertex, v3d::editor::SelectMask::Vertex), 2U);

    // an edge is inside when both ends are, and is found once for its two half edges
    BOOST_CHECK_EQUAL(inside(scene, bottomLeft, topRight, v3d::editor::SelectMask::Edge), 12U);
    BOOST_CHECK_EQUAL(inside(scene, bottomLeft, leftSide, v3d::editor::SelectMask::Edge), 4U);
    BOOST_CHECK_EQUAL(inside(scene, bottomLeft, oneVertex, v3d::editor::SelectMask::Edge), 1U);

    // a face is inside when every vertex of it is
    BOOST_CHECK_EQUAL(inside(scene, bottomLeft, topRight, v3d::editor::SelectMask::Face), 6U);
    BOOST_CHECK_EQUAL(inside(scene, bottomLeft, leftSide, v3d::editor::SelectMask::Face), 1U);
    BOOST_CHECK_EQUAL(inside(scene, bottomLeft, oneVertex, v3d::editor::SelectMask::Face), 0U);
}

BOOST_AUTO_TEST_CASE(picker_empty_test) {
    v3d::editor::Scene scene;
    v3d::editor::Picker picker;

    const boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    BOOST_CHECK_EQUAL(picker.pick(scene, *view, glm::vec2(200.0F, 200.0F),
        v3d::editor::SelectMask::Object).valid, false);

    // a view that has never been given a region divides by zero if it is picked in
    const v3d::type::camera::Profile profile("unsized");
    const v3d::editor::ViewPort unsized("unsized", profile);
    scene.add(boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube()));
    BOOST_CHECK_EQUAL(picker.pick(scene, unsized, glm::vec2(0.0F, 0.0F),
        v3d::editor::SelectMask::Object).valid, false);
    BOOST_CHECK(picker.inside(scene, unsized, glm::vec2(0.0F, 0.0F), glm::vec2(400.0F, 400.0F),
        v3d::editor::SelectMask::Object).empty());
}

};  // namespace

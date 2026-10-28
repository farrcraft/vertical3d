/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/Index.h>
#include <api/brep/Primitives.h>
#include <api/brep/Topology.h>
#include <api/brep/Vertex.h>
#include <api/curve/Curve.h>
#include <api/curve/Curves.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Curve.h>
#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/tool/Picker.h>
#include <vertical3d/src/view/Glyphs.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <optional>
#include <utility>
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
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    const unsigned int id = scene.add(cube);

    const boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;

    // the middle of the view is the middle of the cube
    const v3d::editor::Picker::Hit hit = picker.pick(scene, *view, glm::vec2(200.0F, 200.0F),
        v3d::editor::SelectMask::Object);
    BOOST_CHECK_EQUAL(hit.valid, true);
    BOOST_CHECK_EQUAL(hit.item, id);
    BOOST_CHECK(hit.kind == v3d::editor::SelectMask::Object);

    // the corner of the view is well outside a mesh that covers the middle half of it
    const v3d::editor::Picker::Hit missed = picker.pick(scene, *view, glm::vec2(10.0F, 10.0F),
        v3d::editor::SelectMask::Object);
    BOOST_CHECK_EQUAL(missed.valid, false);
}

BOOST_AUTO_TEST_CASE(picker_nearest_test) {
    v3d::editor::Scene scene;

    // not near/far: the windows headers define both as macros
    const boost::shared_ptr<v3d::editor::Mesh> behind = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    behind->translation(glm::vec3(0.0F, 0.0F, 4.0F));
    scene.add(behind);

    const boost::shared_ptr<v3d::editor::Mesh> front = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    front->translation(glm::vec3(0.0F, 0.0F, 2.0F));
    const unsigned int id = scene.add(front);

    // both are under the cursor, and the one nearer the camera along the ray wins. It is
    // the second added, so insertion order does not decide it
    const boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;
    const v3d::editor::Picker::Hit hit = picker.pick(scene, *view, glm::vec2(200.0F, 200.0F),
        v3d::editor::SelectMask::Object);
    BOOST_CHECK_EQUAL(hit.valid, true);
    BOOST_CHECK_EQUAL(hit.item, id);
}

BOOST_AUTO_TEST_CASE(picker_transform_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
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
    BOOST_CHECK_EQUAL(moved.item, id);
}

BOOST_AUTO_TEST_CASE(picker_beneath_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Group> group = boost::make_shared<v3d::editor::Group>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    group->translation(glm::vec3(-0.5F, 0.5F, 0.0F));
    cube->translation(glm::vec3(0.75F, -0.75F, 0.0F));
    scene.add(group);
    scene.add(cube);
    scene.parent(cube->id(), group->id());

    const boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;

    // a mesh beneath a group is picked where the group puts it, not where its own placement
    // alone would
    BOOST_CHECK(!picker.pick(scene, *view, screen(view, glm::vec3(0.9F, -0.6F, 0.0F)), v3d::editor::SelectMask::Object).valid);
    const v3d::editor::Picker::Hit mesh = picker.pick(scene, *view, screen(view, glm::vec3(0.25F, -0.25F, 0.0F)),
        v3d::editor::SelectMask::Object);
    BOOST_CHECK(mesh.valid);
    BOOST_CHECK_EQUAL(mesh.item, cube->id());

    // a group is picked by its box, and only by a mask that takes every kind
    const glm::vec2 edge = screen(view, glm::vec3(-0.5F + v3d::editor::GROUP_REACH, 0.5F, 0.0F));
    const v3d::editor::Picker::Hit box = picker.pick(scene, *view, edge, v3d::editor::SelectMask::Object);
    BOOST_CHECK(box.valid);
    BOOST_CHECK_EQUAL(box.item, group->id());
    BOOST_CHECK(!picker.pick(scene, *view, edge, v3d::editor::SelectMask::Mesh).valid);
}

BOOST_AUTO_TEST_CASE(picker_face_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    cube->selected(true);
    const unsigned int id = scene.add(cube);

    const boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;

    const v3d::editor::Picker::Hit hit = picker.pick(scene, *view, glm::vec2(200.0F, 200.0F),
        v3d::editor::SelectMask::Face);
    BOOST_CHECK_EQUAL(hit.valid, true);
    BOOST_CHECK_EQUAL(hit.item, id);
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
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
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
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
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
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
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
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    const unsigned int id = scene.add(cube);

    const boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;
    const std::vector<v3d::editor::Picker::Hit> hits = picker.inside(scene, *view,
        screen(view, glm::vec3(-0.6F, -0.6F, 0.0F)), screen(view, glm::vec3(0.6F, 0.6F, 0.0F)),
        v3d::editor::SelectMask::Object);
    BOOST_REQUIRE_EQUAL(hits.size(), 1U);
    BOOST_CHECK_EQUAL(hits[0].valid, true);
    BOOST_CHECK_EQUAL(hits[0].item, id);
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
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
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
    scene.add(boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube()));
    BOOST_CHECK_EQUAL(picker.pick(scene, unsized, glm::vec2(0.0F, 0.0F),
        v3d::editor::SelectMask::Object).valid, false);
    BOOST_CHECK(picker.inside(scene, unsized, glm::vec2(0.0F, 0.0F), glm::vec2(400.0F, 400.0F),
        v3d::editor::SelectMask::Object).empty());
}

/**
 * A cube at the middle of the front view, a point light up and to its right, and a camera
 * down and to its left, each glyph wholly inside the view.
 **/
struct Lit {
    Lit() :
        cube(boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube())),
        light(boost::make_shared<v3d::editor::Light>()),
        camera(boost::make_shared<v3d::editor::Camera>()),
        view(frontView()) {
        // both well inside the view's depth, so every point of either glyph is in front of
        // the eye, and the camera's picture narrow enough to stay clear of the middle
        light->translation(glm::vec3(0.75F, 0.75F, -0.5F));
        camera->translation(glm::vec3(-0.5F, -0.55F, -0.5F));
        v3d::editor::Camera::Lens lens;
        lens.fov = 30.0F;
        camera->lens(lens);
        scene.add(cube);
        scene.add(light);
        scene.add(camera);
    }

    v3d::editor::Scene scene;
    boost::shared_ptr<v3d::editor::Mesh> cube;
    boost::shared_ptr<v3d::editor::Light> light;
    boost::shared_ptr<v3d::editor::Camera> camera;
    boost::shared_ptr<v3d::editor::ViewPort> view;
};

BOOST_FIXTURE_TEST_CASE(picker_glyph_test, Lit) {
    v3d::editor::Picker picker;

    // a click on a line of the light's star hits the light under the object and light masks
    const glm::vec2 onLight = screen(view, light->translation() + glm::vec3(0.1F, 0.0F, 0.0F));
    for (const v3d::editor::SelectMask mask : { v3d::editor::SelectMask::Object, v3d::editor::SelectMask::Light }) {
        const v3d::editor::Picker::Hit hit = picker.pick(scene, *view, onLight, mask);
        BOOST_CHECK(hit.valid);
        BOOST_CHECK_EQUAL(hit.item, light->id());
        BOOST_CHECK(hit.kind == mask);
    }

    // and nothing under the camera mask, which takes only cameras
    BOOST_CHECK(!picker.pick(scene, *view, onLight, v3d::editor::SelectMask::Camera).valid);

    // a click beside the star's lines misses it
    const glm::vec2 beside = screen(view, light->translation() + glm::vec3(0.1F, 0.1F, 0.0F));
    BOOST_CHECK(!picker.pick(scene, *view, beside, v3d::editor::SelectMask::Light).valid);

    // the camera's glyph is hit along a side of its pyramid
    const glm::vec2 onCamera = screen(view, camera->translation());
    const v3d::editor::Picker::Hit hit = picker.pick(scene, *view, onCamera, v3d::editor::SelectMask::Camera);
    BOOST_CHECK(hit.valid);
    BOOST_CHECK_EQUAL(hit.item, camera->id());
}

BOOST_FIXTURE_TEST_CASE(picker_mask_kinds_test, Lit) {
    v3d::editor::Picker picker;
    const glm::vec2 middle(200.0F, 200.0F);

    // the cube is hit under the masks that take meshes, and not under the others
    BOOST_CHECK_EQUAL(picker.pick(scene, *view, middle, v3d::editor::SelectMask::Object).item, cube->id());
    BOOST_CHECK_EQUAL(picker.pick(scene, *view, middle, v3d::editor::SelectMask::Mesh).item, cube->id());
    BOOST_CHECK(picker.pick(scene, *view, middle, v3d::editor::SelectMask::Mesh).kind == v3d::editor::SelectMask::Mesh);
    BOOST_CHECK(!picker.pick(scene, *view, middle, v3d::editor::SelectMask::Light).valid);
    BOOST_CHECK(!picker.pick(scene, *view, middle, v3d::editor::SelectMask::Camera).valid);

    // a component mask never takes a light, selected or not
    light->selected(true);
    const glm::vec2 onLight = screen(view, light->translation() + glm::vec3(0.1F, 0.0F, 0.0F));
    BOOST_CHECK(!picker.pick(scene, *view, onLight, v3d::editor::SelectMask::Vertex).valid);
}

BOOST_FIXTURE_TEST_CASE(picker_inside_glyph_test, Lit) {
    v3d::editor::Picker picker;

    // a rectangle round the whole star holds the light, under the light mask only the light
    const glm::vec2 low = screen(view, light->translation() - glm::vec3(0.3F, 0.3F, 0.0F));
    const glm::vec2 high = screen(view, light->translation() + glm::vec3(0.3F, 0.3F, 0.0F));
    const std::vector<v3d::editor::Picker::Hit> lights = picker.inside(scene, *view, low, high, v3d::editor::SelectMask::Light);
    BOOST_REQUIRE_EQUAL(lights.size(), 1U);
    BOOST_CHECK_EQUAL(lights[0].item, light->id());

    // one round half of it does not
    const glm::vec2 half = screen(view, light->translation());
    BOOST_CHECK(picker.inside(scene, *view, low, half, v3d::editor::SelectMask::Light).empty());

    // the whole view holds everything under the object mask, and only the meshes under the
    // mesh mask
    BOOST_CHECK_EQUAL(picker.inside(scene, *view, glm::vec2(0.0F), glm::vec2(400.0F), v3d::editor::SelectMask::Object).size(), 3U);
    BOOST_CHECK_EQUAL(picker.inside(scene, *view, glm::vec2(0.0F), glm::vec2(400.0F), v3d::editor::SelectMask::Mesh).size(), 1U);
}

BOOST_AUTO_TEST_CASE(picker_curve_test) {
    // a curve across the view, from the middle of its left half to the middle of its right
    const boost::shared_ptr<v3d::curve::Curves> curves = boost::make_shared<v3d::curve::Curves>();
    BOOST_REQUIRE(curves->add({ .shape = v3d::curve::Poly{ { { -0.5F, 0.0F, 0.0F }, { 0.5F, 0.0F, 0.0F } } } }).empty());
    v3d::editor::Scene scene;
    const unsigned int id = scene.add(boost::make_shared<v3d::editor::Curve>(curves));
    const boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;

    // a click on the line picks it under the curve mask and the object mask
    const glm::vec2 on = screen(view, glm::vec3(0.25F, 0.0F, 0.0F));
    const v3d::editor::Picker::Hit hit = picker.pick(scene, *view, on, v3d::editor::SelectMask::Curve);
    BOOST_CHECK(hit.valid);
    BOOST_CHECK_EQUAL(hit.item, id);
    BOOST_CHECK(picker.pick(scene, *view, on, v3d::editor::SelectMask::Object).valid);

    // the mesh mask does not take a curve, and a click well off the line misses it
    BOOST_CHECK(!picker.pick(scene, *view, on, v3d::editor::SelectMask::Mesh).valid);
    const glm::vec2 off = screen(view, glm::vec3(0.25F, 0.5F, 0.0F));
    BOOST_CHECK(!picker.pick(scene, *view, off, v3d::editor::SelectMask::Curve).valid);
}

BOOST_AUTO_TEST_CASE(picker_curve_parts_test) {
    // a poly curve, then a Bezier curve whose two points have handles a quarter unit to either
    // side of them along y
    const boost::shared_ptr<v3d::curve::Curves> curves = boost::make_shared<v3d::curve::Curves>();
    BOOST_REQUIRE(curves->add({ .shape = v3d::curve::Poly{ { { -0.5F, 0.5F, 0.0F }, { 0.5F, 0.5F, 0.0F } } } }).empty());
    v3d::curve::Bezier bezier;
    for (const float x : { -0.5F, 0.5F }) {
        bezier.points.push_back({ .position = { x, -0.5F, 0.0F }, .left = { x, -0.75F, 0.0F }, .right = { x, -0.25F, 0.0F },
            .leftType = v3d::curve::Handle::Free, .rightType = v3d::curve::Handle::Free });
    }
    BOOST_REQUIRE(curves->add({ .shape = bezier }).empty());
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Curve> curve = boost::make_shared<v3d::editor::Curve>(curves);
    const unsigned int id = scene.add(curve);
    const boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::Picker picker;

    // an unselected curve offers none of its parts
    const glm::vec2 second = screen(view, glm::vec3(-0.5F, -0.5F, 0.0F));
    BOOST_CHECK(!picker.pick(scene, *view, second, v3d::editor::SelectMask::Vertex).valid);

    // the vertex mask finds control points, numbered on from the curves before
    curve->selected(true);
    const v3d::editor::Picker::Hit point = picker.pick(scene, *view, second, v3d::editor::SelectMask::Vertex);
    BOOST_CHECK(point.valid);
    BOOST_CHECK_EQUAL(point.item, id);
    BOOST_CHECK_EQUAL(point.component, 2U);
    // a handle is not a control point, and a control point is not a handle
    const glm::vec2 handle = screen(view, glm::vec3(0.5F, -0.25F, 0.0F));
    BOOST_CHECK(!picker.pick(scene, *view, handle, v3d::editor::SelectMask::Vertex).valid);
    BOOST_CHECK(!picker.pick(scene, *view, second, v3d::editor::SelectMask::Handle).valid);

    // the handle mask finds the right handle of the curve's fourth point, twice its number and one
    const v3d::editor::Picker::Hit right = picker.pick(scene, *view, handle, v3d::editor::SelectMask::Handle);
    BOOST_CHECK(right.valid);
    BOOST_CHECK_EQUAL(right.component, 7U);
    // the poly curve's points have no handles to find
    BOOST_CHECK(!picker.pick(scene, *view, screen(view, glm::vec3(-0.5F, 0.5F, 0.0F)), v3d::editor::SelectMask::Handle).valid);

    // a band over the lower half holds the Bezier curve's points, and all four of its handles
    const std::pair<glm::vec2, glm::vec2> band(screen(view, glm::vec3(-0.9F, -0.05F, 0.0F)), screen(view, glm::vec3(0.9F, -0.95F, 0.0F)));
    const std::vector<v3d::editor::Picker::Hit> points = picker.inside(scene, *view, band.first,
        band.second, v3d::editor::SelectMask::Vertex);
    BOOST_REQUIRE_EQUAL(points.size(), 2U);
    BOOST_CHECK_EQUAL(points[0].component, 2U);
    BOOST_CHECK_EQUAL(points[1].component, 3U);
    BOOST_CHECK_EQUAL(picker.inside(scene, *view, band.first, band.second,
        v3d::editor::SelectMask::Handle).size(), 4U);
}

};  // namespace

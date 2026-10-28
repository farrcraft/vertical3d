/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/Primitives.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/manipulator/Manipulator.h>
#include <vertical3d/src/manipulator/RotateManipulator.h>
#include <vertical3d/src/manipulator/ScaleManipulator.h>
#include <vertical3d/src/manipulator/Screen.h>
#include <vertical3d/src/manipulator/Target.h>
#include <vertical3d/src/manipulator/TranslateManipulator.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/view/ViewPort.h>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

/**
 * The same square front view the picker's own tests use: a world unit is two hundred
 * pixels and the middle of the view is the middle of a unit primitive. The camera looks
 * along +z, so world x runs right across the view and world y runs up it.
 **/
v3d::editor::Screen frontView() {
    const v3d::type::camera::Profile profile("front");
    v3d::editor::ViewPort view("front", profile);
    view.resize(glm::vec4(0.0F, 0.0F, 400.0F, 400.0F));
    return v3d::editor::Screen{ .camera = view.camera(), .region = view.region() };
}

/**
 * The middle of that view. An untranslated mesh sits there.
 **/
constexpr glm::vec2 centre(200.0F, 200.0F);

/**
 * Whole objects as a manipulator's target.
 **/
v3d::editor::Target objects(const v3d::editor::Items& items) {
    return v3d::editor::Target::objects(items, v3d::editor::Evaluation());
}

/**
 * One motion event's worth of drag of whole objects, from one cursor position to another.
 **/
void drag(const v3d::editor::Manipulator& manipulator, const v3d::editor::Items& items, const v3d::editor::Screen& view,
    const glm::vec2& from, const glm::vec2& to) {
    v3d::editor::Target target = objects(items);
    manipulator.apply(&target, view, from, to);
}

BOOST_AUTO_TEST_CASE(manipulator_default_test) {
    const v3d::editor::TranslateManipulator manipulator;

    // both halves of the state are initialised. A manipulator whose coordinate space was
    // never written would be in one or the other at random
    BOOST_CHECK(manipulator.axis() == v3d::editor::Manipulator::Axis::None);
    BOOST_CHECK(manipulator.space() == v3d::editor::Manipulator::Space::Global);
    BOOST_CHECK_EQUAL(manipulator.active(), false);
}

BOOST_AUTO_TEST_CASE(manipulator_grab_test) {
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    const v3d::editor::Screen view = frontView();
    const v3d::editor::TranslateManipulator manipulator;

    // the centre handle sits where all three axes meet, so a click on the origin finds it
    // rather than whichever axis happened to be tested first
    v3d::editor::Manipulator::Axis axis = v3d::editor::Manipulator::Axis::X;
    BOOST_CHECK_EQUAL(manipulator.grab(objects({cube}), view, centre, &axis), true);
    BOOST_CHECK(axis == v3d::editor::Manipulator::Axis::None);

    // world x runs right across this view, so the x handle is out along it
    const v3d::editor::Manipulator::Placement seat = manipulator.placement(objects({cube}), view);
    BOOST_CHECK_EQUAL(seat.valid, true);
    BOOST_CHECK_EQUAL(manipulator.grab(objects({cube}), view, glm::vec2(centre.x + 45.0F, centre.y), &axis), true);
    BOOST_CHECK(axis == v3d::editor::Manipulator::Axis::X);

    // and world y runs up it, which a screen coordinate measures downwards
    BOOST_CHECK_EQUAL(manipulator.grab(objects({cube}), view, glm::vec2(centre.x, centre.y - 45.0F), &axis), true);
    BOOST_CHECK(axis == v3d::editor::Manipulator::Axis::Y);

    // nowhere near a handle
    BOOST_CHECK_EQUAL(manipulator.grab(objects({cube}), view, glm::vec2(30.0F, 370.0F), &axis), false);

    // nothing selected is nothing to grab
    BOOST_CHECK_EQUAL(manipulator.grab(objects({}), view, centre, &axis), false);
}

BOOST_AUTO_TEST_CASE(manipulator_translate_axis_test) {
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    const v3d::editor::Screen view = frontView();
    v3d::editor::TranslateManipulator manipulator;
    manipulator.axis(v3d::editor::Manipulator::Axis::X);
    manipulator.active(true);

    // a world unit is two hundred pixels in this view, so a hundred pixel drag along the x
    // handle is half a unit
    drag(manipulator, {cube}, view, centre, glm::vec2(centre.x + 100.0F, centre.y));
    BOOST_CHECK_CLOSE(cube->translation().x, 0.5F, 0.5F);
    BOOST_CHECK_SMALL(cube->translation().y, 0.001F);
    BOOST_CHECK_SMALL(cube->translation().z, 0.001F);

    // a drag is an offset and not a position: the second one adds to the first rather than
    // moving the object back to where one gesture's worth of delta would put it
    drag(manipulator, {cube}, view, centre, glm::vec2(centre.x + 100.0F, centre.y));
    BOOST_CHECK_CLOSE(cube->translation().x, 1.0F, 0.5F);

    // the drag's component across the handle does nothing, so the drag is constrained to
    // the handle
    const glm::vec3 before = cube->translation();
    drag(manipulator, {cube}, view, centre, glm::vec2(centre.x, centre.y + 100.0F));
    BOOST_CHECK_CLOSE(cube->translation().x, before.x, 0.5F);
}

BOOST_AUTO_TEST_CASE(manipulator_translate_free_test) {
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    const v3d::editor::Screen view = frontView();
    v3d::editor::TranslateManipulator manipulator;
    manipulator.axis(v3d::editor::Manipulator::Axis::None);
    manipulator.active(true);

    // both components of the gesture move the object. A drag right and up the screen is a
    // move along world +x and +y, screen y being measured downwards
    drag(manipulator, {cube}, view, centre, glm::vec2(centre.x + 100.0F, centre.y - 40.0F));
    BOOST_CHECK_CLOSE(cube->translation().x, 0.5F, 0.5F);
    BOOST_CHECK_CLOSE(cube->translation().y, 0.2F, 0.5F);
    BOOST_CHECK_SMALL(cube->translation().z, 0.001F);
}

BOOST_AUTO_TEST_CASE(manipulator_rotate_grab_test) {
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    const v3d::editor::Screen view = frontView();
    const v3d::editor::RotateManipulator manipulator;

    // this view looks along z, so the z ring faces the camera and the other two are edge on.
    // The rim of the z ring at three o'clock is also where the edge on y ring's projection
    // ends, and the click goes to the ring that can be seen
    const v3d::editor::Manipulator::Placement seat = manipulator.placement(objects({cube}), view);
    const float radius = seat.size * 200.0F;
    v3d::editor::Manipulator::Axis axis = v3d::editor::Manipulator::Axis::None;
    BOOST_CHECK_EQUAL(manipulator.grab(objects({cube}), view, glm::vec2(centre.x + radius, centre.y), &axis), true);
    BOOST_CHECK(axis == v3d::editor::Manipulator::Axis::Z);

    // and the same at twelve o'clock, where the edge on x ring ends
    BOOST_CHECK_EQUAL(manipulator.grab(objects({cube}), view, glm::vec2(centre.x, centre.y - radius), &axis), true);
    BOOST_CHECK(axis == v3d::editor::Manipulator::Axis::Z);

    // a ring is grabbed anywhere along it and not only where it is drawn through a point:
    // the arc between two of those is wider than the tolerance
    BOOST_CHECK_EQUAL(manipulator.grab(objects({cube}), view,
        glm::vec2(centre.x + (radius * 0.7071F), centre.y - (radius * 0.7071F)), &axis), true);
    BOOST_CHECK(axis == v3d::editor::Manipulator::Axis::Z);

    // inside the ring and away from the centre handle is nothing
    BOOST_CHECK_EQUAL(manipulator.grab(objects({cube}), view, glm::vec2(centre.x + (radius * 0.5F), centre.y), &axis), false);
}

BOOST_AUTO_TEST_CASE(manipulator_rotate_test) {
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    const v3d::editor::Screen view = frontView();
    v3d::editor::RotateManipulator manipulator;
    manipulator.axis(v3d::editor::Manipulator::Axis::Z);
    manipulator.active(true);

    // a quarter sweep round the origin, from the right of it to below it. The z axis runs
    // into this view, so the world turn is the opposite way round from the screen one
    drag(manipulator, {cube}, view, glm::vec2(centre.x + 90.0F, centre.y), glm::vec2(centre.x, centre.y + 90.0F));

    // the object's own x axis, drawn pointing right, follows the cursor down the screen,
    // and down the screen is world -y
    const glm::vec3 turned = cube->rotation() * glm::vec3(1.0F, 0.0F, 0.0F);
    BOOST_CHECK_SMALL(turned.x, 0.001F);
    BOOST_CHECK_CLOSE(turned.y, -1.0F, 0.5F);

    // a second sweep composes onto the first rather than replacing it
    drag(manipulator, {cube}, view, glm::vec2(centre.x, centre.y + 90.0F), glm::vec2(centre.x - 90.0F, centre.y));
    const glm::vec3 again = cube->rotation() * glm::vec3(1.0F, 0.0F, 0.0F);
    BOOST_CHECK_CLOSE(again.x, -1.0F, 0.5F);
    BOOST_CHECK_SMALL(again.y, 0.001F);
}

BOOST_AUTO_TEST_CASE(manipulator_scale_test) {
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    const v3d::editor::Screen view = frontView();
    v3d::editor::ScaleManipulator manipulator;

    // a scale is a vector in the object's own axes, so the handles follow the object's axes
    // whatever the coordinate space says. A quarter turn about z puts the x handle along
    // world y, even in the global space
    cube->rotation(glm::angleAxis(glm::half_pi<float>(), glm::vec3(0.0F, 0.0F, 1.0F)));
    manipulator.space(v3d::editor::Manipulator::Space::Global);
    const glm::vec3 handleAxis = manipulator.placement(objects({cube}), view).orientation * glm::vec3(1.0F, 0.0F, 0.0F);
    BOOST_CHECK_SMALL(handleAxis.x, 0.001F);
    BOOST_CHECK_CLOSE(handleAxis.y, 1.0F, 0.5F);
    cube->rotation(glm::quat(1.0F, 0.0F, 0.0F, 0.0F));

    manipulator.axis(v3d::editor::Manipulator::Axis::X);
    manipulator.active(true);

    // dragging a handle out by its own length doubles the object along it, whatever the
    // object's size in world units
    const v3d::editor::Manipulator::Placement seat = manipulator.placement(objects({cube}), view);
    const float handle = seat.size * 200.0F;
    drag(manipulator, {cube}, view, centre, glm::vec2(centre.x + handle, centre.y));
    BOOST_CHECK_CLOSE(cube->scale().x, 2.0F, 1.0F);
    BOOST_CHECK_CLOSE(cube->scale().y, 1.0F, 0.5F);
    BOOST_CHECK_CLOSE(cube->scale().z, 1.0F, 0.5F);

    // a drag far enough to run the scale past zero stops short of it: an inside out object
    // has no handle left to drag back
    drag(manipulator, {cube}, view, centre, glm::vec2(centre.x - (handle * 20.0F), centre.y));
    BOOST_CHECK_GT(cube->scale().x, 0.0F);
}

BOOST_AUTO_TEST_CASE(manipulator_pivot_test) {
    const boost::shared_ptr<v3d::editor::Mesh> first = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    const boost::shared_ptr<v3d::editor::Mesh> second = boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube());
    first->translation(glm::vec3(1.0F, 0.0F, 0.0F));
    second->translation(glm::vec3(3.0F, 2.0F, 0.0F));
    const v3d::editor::Screen view = frontView();
    const v3d::editor::ScaleManipulator manipulator;

    // the handles sit at the mean of the origins
    const v3d::editor::Manipulator::Placement seat = manipulator.placement(objects({first, second}), view);
    BOOST_CHECK_EQUAL(seat.valid, true);
    BOOST_CHECK_CLOSE(seat.origin.x, 2.0F, 0.01F);
    BOOST_CHECK_CLOSE(seat.origin.y, 1.0F, 0.01F);

    // an empty entry is not an origin, and an empty set has no handles
    const v3d::editor::Manipulator::Placement skipped =
        manipulator.placement(objects({first, boost::shared_ptr<v3d::editor::Item>()}), view);
    BOOST_CHECK_CLOSE(skipped.origin.x, 1.0F, 0.01F);
    BOOST_CHECK_EQUAL(manipulator.placement(objects({}), view).valid, false);

    // in the object's own axes, the handles follow the first object of the set
    first->rotation(glm::angleAxis(glm::half_pi<float>(), glm::vec3(0.0F, 0.0F, 1.0F)));
    const glm::vec3 handleAxis = manipulator.placement(objects({first, second}), view).orientation * glm::vec3(1.0F, 0.0F, 0.0F);
    BOOST_CHECK_SMALL(handleAxis.x, 0.001F);
    BOOST_CHECK_CLOSE(handleAxis.y, 1.0F, 0.5F);
}

/**
 * Two cubes either side of the middle of the front view, so the pivot is the middle.
 **/
struct TwoCubes {
    TwoCubes() :
        left(boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube())),
        right(boost::make_shared<v3d::editor::Mesh>(v3d::brep::cube())),
        view(frontView()) {
        left->translation(glm::vec3(-0.5F, 0.0F, 0.0F));
        right->translation(glm::vec3(0.5F, 0.0F, 0.0F));
    }

    boost::shared_ptr<v3d::editor::Mesh> left;
    boost::shared_ptr<v3d::editor::Mesh> right;
    v3d::editor::Screen view;
};

BOOST_AUTO_TEST_CASE(manipulator_set_translate_test) {
    const TwoCubes cubes;
    v3d::editor::TranslateManipulator manipulator;
    manipulator.axis(v3d::editor::Manipulator::Axis::X);
    manipulator.active(true);

    // every object moves by the same offset
    drag(manipulator, {cubes.left, cubes.right}, cubes.view, centre, glm::vec2(centre.x + 100.0F, centre.y));
    BOOST_CHECK_SMALL(cubes.left->translation().x, 0.01F);
    BOOST_CHECK_CLOSE(cubes.right->translation().x, 1.0F, 1.0F);
}

BOOST_AUTO_TEST_CASE(manipulator_set_rotate_test) {
    const TwoCubes cubes;
    v3d::editor::RotateManipulator manipulator;
    manipulator.axis(v3d::editor::Manipulator::Axis::Z);
    manipulator.active(true);

    // the quarter turn that carries the x axis to world -y carries each origin round the
    // pivot with it, and turns each object as it would turn one alone
    drag(manipulator, {cubes.left, cubes.right}, cubes.view, glm::vec2(centre.x + 90.0F, centre.y),
        glm::vec2(centre.x, centre.y + 90.0F));
    BOOST_CHECK_SMALL(cubes.left->translation().x, 0.001F);
    BOOST_CHECK_CLOSE(cubes.left->translation().y, 0.5F, 0.5F);
    BOOST_CHECK_SMALL(cubes.right->translation().x, 0.001F);
    BOOST_CHECK_CLOSE(cubes.right->translation().y, -0.5F, 0.5F);
    const glm::vec3 turned = cubes.right->rotation() * glm::vec3(1.0F, 0.0F, 0.0F);
    BOOST_CHECK_CLOSE(turned.y, -1.0F, 0.5F);
    BOOST_CHECK(cubes.left->rotation() == cubes.right->rotation());
}

BOOST_AUTO_TEST_CASE(manipulator_set_scale_test) {
    const TwoCubes cubes;
    v3d::editor::ScaleManipulator manipulator;
    manipulator.axis(v3d::editor::Manipulator::Axis::X);
    manipulator.active(true);

    // doubling along x doubles each object and its distance from the pivot along x
    const float handle = manipulator.placement(objects({cubes.left, cubes.right}), cubes.view).size * 200.0F;
    drag(manipulator, {cubes.left, cubes.right}, cubes.view, centre, glm::vec2(centre.x + handle, centre.y));
    BOOST_CHECK_CLOSE(cubes.left->translation().x, -1.0F, 1.0F);
    BOOST_CHECK_CLOSE(cubes.right->translation().x, 1.0F, 1.0F);
    BOOST_CHECK_CLOSE(cubes.left->scale().x, 2.0F, 1.0F);
    BOOST_CHECK_CLOSE(cubes.right->scale().y, 1.0F, 0.5F);

    // the centre handle scales the distance all three ways
    cubes.left->translation(glm::vec3(-0.5F, 0.5F, 0.0F));
    cubes.right->translation(glm::vec3(0.5F, -0.5F, 0.0F));
    manipulator.axis(v3d::editor::Manipulator::Axis::None);
    drag(manipulator, {cubes.left, cubes.right}, cubes.view, centre, glm::vec2(centre.x + 90.0F, centre.y));
    BOOST_CHECK_CLOSE(cubes.left->translation().x, -1.0F, 1.0F);
    BOOST_CHECK_CLOSE(cubes.left->translation().y, 1.0F, 1.0F);

    // a drag past zero leaves the objects on their own sides of the pivot
    manipulator.axis(v3d::editor::Manipulator::Axis::X);
    drag(manipulator, {cubes.left, cubes.right}, cubes.view, centre, glm::vec2(centre.x - (handle * 20.0F), centre.y));
    BOOST_CHECK_LT(cubes.left->translation().x, 0.0F);
    BOOST_CHECK_GT(cubes.right->translation().x, 0.0F);
}

};  // namespace

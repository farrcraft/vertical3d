/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "../../src/scene/CreatePoly.h"
#include "../../src/manipulator/RotateManipulator.h"
#include "../../src/manipulator/ScaleManipulator.h"
#include "../../src/manipulator/TranslateManipulator.h"
#include "../../src/view/ViewPort.h"

namespace {

    /**
     * The same square front view the picker's own tests use: a world unit is two hundred
     * pixels and the middle of the view is the middle of a unit primitive. The camera looks
     * along +z, so world x runs right across the view and world y runs up it.
     **/
    boost::shared_ptr<v3d::editor::ViewPort> frontView() {
        v3d::type::CameraProfile profile("front");
        boost::shared_ptr<v3d::editor::ViewPort> view = boost::make_shared<v3d::editor::ViewPort>("front", profile);
        view->resize(glm::vec4(0.0f, 0.0f, 400.0f, 400.0f));
        return view;
    }

    /**
     * The middle of that view, which is where an untranslated mesh sits.
     **/
    const glm::vec2 centre(200.0f, 200.0f);

};  // namespace

BOOST_AUTO_TEST_CASE(manipulator_default_test) {
    v3d::editor::TranslateManipulator manipulator;

    // both halves of the state are initialised - a manipulator whose coordinate space was
    // never written would be in one or the other at random
    BOOST_CHECK(manipulator.axis() == v3d::editor::Manipulator::Axis::None);
    BOOST_CHECK(manipulator.space() == v3d::editor::Manipulator::Space::Global);
    BOOST_CHECK_EQUAL(manipulator.active(), false);
}

BOOST_AUTO_TEST_CASE(manipulator_grab_test) {
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::TranslateManipulator manipulator;

    // the centre handle sits where all three axes meet, so it is what a click on the origin
    // finds rather than whichever axis happened to be tested first
    v3d::editor::Manipulator::Axis axis = v3d::editor::Manipulator::Axis::X;
    BOOST_CHECK_EQUAL(manipulator.grab(cube, *view, centre, &axis), true);
    BOOST_CHECK(axis == v3d::editor::Manipulator::Axis::None);

    // world x runs right across this view, so the x handle is out along it
    const v3d::editor::Manipulator::Placement seat = manipulator.placement(cube, *view);
    BOOST_CHECK_EQUAL(seat.valid, true);
    BOOST_CHECK_EQUAL(manipulator.grab(cube, *view, glm::vec2(centre.x + 45.0f, centre.y), &axis), true);
    BOOST_CHECK(axis == v3d::editor::Manipulator::Axis::X);

    // and world y runs up it, which a screen coordinate measures downwards
    BOOST_CHECK_EQUAL(manipulator.grab(cube, *view, glm::vec2(centre.x, centre.y - 45.0f), &axis), true);
    BOOST_CHECK(axis == v3d::editor::Manipulator::Axis::Y);

    // nowhere near a handle
    BOOST_CHECK_EQUAL(manipulator.grab(cube, *view, glm::vec2(30.0f, 370.0f), &axis), false);

    // nothing selected is nothing to grab
    BOOST_CHECK_EQUAL(manipulator.grab(boost::shared_ptr<v3d::brep::BRep>(), *view, centre, &axis), false);
}

BOOST_AUTO_TEST_CASE(manipulator_translate_axis_test) {
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::TranslateManipulator manipulator;
    manipulator.axis(v3d::editor::Manipulator::Axis::X);
    manipulator.active(true);

    // a world unit is two hundred pixels in this view, so a hundred pixel drag along the x
    // handle is half a unit
    manipulator.apply(cube, *view, centre, glm::vec2(centre.x + 100.0f, centre.y));
    BOOST_CHECK_CLOSE(cube->translation().x, 0.5f, 0.5f);
    BOOST_CHECK_SMALL(cube->translation().y, 0.001f);
    BOOST_CHECK_SMALL(cube->translation().z, 0.001f);

    // a drag is an offset and not a position: the second one adds to the first rather than
    // moving the object back to where one gesture's worth of delta would put it
    manipulator.apply(cube, *view, centre, glm::vec2(centre.x + 100.0f, centre.y));
    BOOST_CHECK_CLOSE(cube->translation().x, 1.0f, 0.5f);

    // the drag's component across the handle does nothing, which is what constrains it
    const glm::vec3 before = cube->translation();
    manipulator.apply(cube, *view, centre, glm::vec2(centre.x, centre.y + 100.0f));
    BOOST_CHECK_CLOSE(cube->translation().x, before.x, 0.5f);
}

BOOST_AUTO_TEST_CASE(manipulator_translate_free_test) {
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::TranslateManipulator manipulator;
    manipulator.axis(v3d::editor::Manipulator::Axis::None);
    manipulator.active(true);

    // both components of the gesture move the object. A drag right and up the screen is a
    // move along world +x and +y, screen y being measured downwards
    manipulator.apply(cube, *view, centre, glm::vec2(centre.x + 100.0f, centre.y - 40.0f));
    BOOST_CHECK_CLOSE(cube->translation().x, 0.5f, 0.5f);
    BOOST_CHECK_CLOSE(cube->translation().y, 0.2f, 0.5f);
    BOOST_CHECK_SMALL(cube->translation().z, 0.001f);
}

BOOST_AUTO_TEST_CASE(manipulator_rotate_grab_test) {
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::RotateManipulator manipulator;

    // this view looks along z, so the z ring faces the camera and the other two are edge on.
    // The rim of the z ring at three o'clock is also where the edge on y ring's projection
    // ends, and the click is meant for the ring that can be seen
    const v3d::editor::Manipulator::Placement seat = manipulator.placement(cube, *view);
    const float radius = seat.size * 200.0f;
    v3d::editor::Manipulator::Axis axis = v3d::editor::Manipulator::Axis::None;
    BOOST_CHECK_EQUAL(manipulator.grab(cube, *view, glm::vec2(centre.x + radius, centre.y), &axis), true);
    BOOST_CHECK(axis == v3d::editor::Manipulator::Axis::Z);

    // and the same at twelve o'clock, where the edge on x ring ends
    BOOST_CHECK_EQUAL(manipulator.grab(cube, *view, glm::vec2(centre.x, centre.y - radius), &axis), true);
    BOOST_CHECK(axis == v3d::editor::Manipulator::Axis::Z);

    // a ring is grabbed anywhere along it and not only where it is drawn through a point:
    // the arc between two of those is wider than the tolerance
    BOOST_CHECK_EQUAL(manipulator.grab(cube, *view,
        glm::vec2(centre.x + radius * 0.7071f, centre.y - radius * 0.7071f), &axis), true);
    BOOST_CHECK(axis == v3d::editor::Manipulator::Axis::Z);

    // inside the ring and away from the centre handle is nothing
    BOOST_CHECK_EQUAL(manipulator.grab(cube, *view, glm::vec2(centre.x + radius * 0.5f, centre.y), &axis), false);
}

BOOST_AUTO_TEST_CASE(manipulator_rotate_test) {
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::RotateManipulator manipulator;
    manipulator.axis(v3d::editor::Manipulator::Axis::Z);
    manipulator.active(true);

    // a quarter sweep round the origin, from the right of it to below it. The z axis runs
    // into this view, so the world turn is the opposite way round from the screen one
    manipulator.apply(cube, *view, glm::vec2(centre.x + 90.0f, centre.y), glm::vec2(centre.x, centre.y + 90.0f));

    // the object's own x axis, drawn pointing right, follows the cursor down the screen -
    // and down the screen is world -y
    const glm::vec3 turned = cube->rotation() * glm::vec3(1.0f, 0.0f, 0.0f);
    BOOST_CHECK_SMALL(turned.x, 0.001f);
    BOOST_CHECK_CLOSE(turned.y, -1.0f, 0.5f);

    // a second sweep composes onto the first rather than replacing it
    manipulator.apply(cube, *view, glm::vec2(centre.x, centre.y + 90.0f), glm::vec2(centre.x - 90.0f, centre.y));
    const glm::vec3 again = cube->rotation() * glm::vec3(1.0f, 0.0f, 0.0f);
    BOOST_CHECK_CLOSE(again.x, -1.0f, 0.5f);
    BOOST_CHECK_SMALL(again.y, 0.001f);
}

BOOST_AUTO_TEST_CASE(manipulator_scale_test) {
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    boost::shared_ptr<v3d::editor::ViewPort> view = frontView();
    v3d::editor::ScaleManipulator manipulator;

    // a scale is a vector in the object's own axes, so the handles are the object's axes
    // whatever the coordinate space says - a quarter turn about z puts the x handle along
    // world y even in the global space
    cube->rotation(glm::angleAxis(glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f)));
    manipulator.space(v3d::editor::Manipulator::Space::Global);
    const glm::vec3 handleAxis = manipulator.placement(cube, *view).orientation * glm::vec3(1.0f, 0.0f, 0.0f);
    BOOST_CHECK_SMALL(handleAxis.x, 0.001f);
    BOOST_CHECK_CLOSE(handleAxis.y, 1.0f, 0.5f);
    cube->rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

    manipulator.axis(v3d::editor::Manipulator::Axis::X);
    manipulator.active(true);

    // dragging a handle out by its own length doubles the object along it, whatever the
    // object's size in world units
    const v3d::editor::Manipulator::Placement seat = manipulator.placement(cube, *view);
    const float handle = seat.size * 200.0f;
    manipulator.apply(cube, *view, centre, glm::vec2(centre.x + handle, centre.y));
    BOOST_CHECK_CLOSE(cube->scale().x, 2.0f, 1.0f);
    BOOST_CHECK_CLOSE(cube->scale().y, 1.0f, 0.5f);
    BOOST_CHECK_CLOSE(cube->scale().z, 1.0f, 0.5f);

    // a drag far enough to run the scale past zero stops short of it: an inside out object
    // has no handle left to drag back
    manipulator.apply(cube, *view, centre, glm::vec2(centre.x - handle * 20.0f, centre.y));
    BOOST_CHECK_GT(cube->scale().x, 0.0f);
}

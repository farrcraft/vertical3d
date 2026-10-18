/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/camera/Camera.h>
#include <api/type/camera/Isometric.h>

#include <cmath>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>

namespace {

const int VIEWPORT_WIDTH = 800;
const int VIEWPORT_HEIGHT = 600;

/**
 * A camera the orbit has been written onto, with its matrices built and a viewport that is
 * not square - a square one would hide an aspect ratio applied to the wrong axis.
 **/
v3d::type::camera::Camera applied(const v3d::type::camera::Isometric& orbit) {
    v3d::type::camera::Camera camera;
    camera.profile().pixelAspect(static_cast<float>(VIEWPORT_WIDTH) / static_cast<float>(VIEWPORT_HEIGHT));
    camera.profile().clipping(glm::vec2(0.1F, 200.0F));
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();
    return camera;
}

glm::vec3 screen(v3d::type::camera::Camera* camera, const glm::vec3& point) {
    int viewport[4] = { 0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT };
    return camera->project(point, viewport);
}

BOOST_AUTO_TEST_CASE(isometriccamera_defaults_test) {
    const v3d::type::camera::Isometric orbit;

    BOOST_CHECK_EQUAL(orbit.azimuth(), 0);
    BOOST_CHECK_CLOSE(orbit.zoom(), v3d::type::camera::Isometric::DEFAULT_ZOOM, 0.01F);
    BOOST_CHECK_CLOSE(orbit.elevation(), v3d::type::camera::Isometric::DEFAULT_ELEVATION, 0.01F);
    BOOST_CHECK_CLOSE(orbit.distance(), v3d::type::camera::Isometric::DEFAULT_DISTANCE, 0.01F);
    BOOST_CHECK_EQUAL(orbit.target().x, 0.0F);
    BOOST_CHECK_EQUAL(orbit.target().y, 0.0F);
    BOOST_CHECK_EQUAL(orbit.target().z, 0.0F);
}

BOOST_AUTO_TEST_CASE(isometriccamera_rotation_wraps_test) {
    v3d::type::camera::Isometric orbit;

    for (int step = 1; step < v3d::type::camera::Isometric::AZIMUTHS; ++step) {
        orbit.rotate(1);
        BOOST_CHECK_EQUAL(orbit.azimuth(), step);
    }
    // four steps is a whole turn, so the fourth comes back to where it started
    orbit.rotate(1);
    BOOST_CHECK_EQUAL(orbit.azimuth(), 0);

    // and the other way, which the built in remainder gets wrong on its own
    orbit.rotate(-1);
    BOOST_CHECK_EQUAL(orbit.azimuth(), 3);

    orbit.azimuth(9);
    BOOST_CHECK_EQUAL(orbit.azimuth(), 1);
    orbit.azimuth(-1);
    BOOST_CHECK_EQUAL(orbit.azimuth(), 3);
}

BOOST_AUTO_TEST_CASE(isometriccamera_eye_is_on_the_orbit_test) {
    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(3.0F, 0.0F, -4.0F));

    for (int index = 0; index < v3d::type::camera::Isometric::AZIMUTHS; ++index) {
        orbit.azimuth(index);
        const glm::vec3 offset = orbit.eye() - orbit.target();

        // the eye is one distance from the target whichever corner it is at, and the same
        // height above it, so the four views are comparable
        BOOST_CHECK_CLOSE(glm::length(offset), orbit.distance(), 0.01F);
        BOOST_CHECK_CLOSE(offset.y, std::sin(orbit.elevation()) * orbit.distance(), 0.01F);
    }
}

BOOST_AUTO_TEST_CASE(isometriccamera_axes_are_on_the_ground_test) {
    v3d::type::camera::Isometric orbit;

    for (int index = 0; index < v3d::type::camera::Isometric::AZIMUTHS; ++index) {
        orbit.azimuth(index);

        // both axes lie in the ground plane, are unit length and are at right angles, so a
        // pan cannot lift the target off the plane or scale with the direction it is in
        BOOST_CHECK_SMALL(orbit.right().y, 0.0001F);
        BOOST_CHECK_SMALL(orbit.forward().y, 0.0001F);
        BOOST_CHECK_CLOSE(glm::length(orbit.right()), 1.0F, 0.01F);
        BOOST_CHECK_CLOSE(glm::length(orbit.forward()), 1.0F, 0.01F);
        BOOST_CHECK_SMALL(glm::dot(orbit.right(), orbit.forward()), 0.0001F);
    }
}

BOOST_AUTO_TEST_CASE(isometriccamera_a_quarter_turn_turns_the_axes_test) {
    v3d::type::camera::Isometric orbit;

    const glm::vec3 forward = orbit.forward();
    const glm::vec3 right = orbit.right();

    orbit.rotate(1);
    // a quarter turn counterclockwise takes forward onto where right was pointing back
    BOOST_CHECK_SMALL(glm::length(orbit.forward() + right), 0.0001F);
    BOOST_CHECK_SMALL(glm::length(orbit.right() - forward), 0.0001F);

    orbit.rotate(2);
    // and a half turn from there reverses both
    BOOST_CHECK_SMALL(glm::length(orbit.forward() - right), 0.0001F);
    BOOST_CHECK_SMALL(glm::length(orbit.right() + forward), 0.0001F);
}

/**
 * right() is to the right on screen and forward() is away, at every azimuth and in either
 * hand. The hand changes the ground vectors and the view together, so the screen direction
 * stays fixed while the basis changes. This catches an orbit whose right() follows one hand
 * while apply() draws in the other.
 **/
BOOST_AUTO_TEST_CASE(isometriccamera_right_is_to_the_right_on_screen_test) {
    const v3d::type::camera::Profile::Hand hands[] = {
        v3d::type::camera::Profile::Hand::UpCrossDirection,
        v3d::type::camera::Profile::Hand::DirectionCrossUp
    };

    for (const v3d::type::camera::Profile::Hand hand : hands) {
        v3d::type::camera::Isometric orbit;
        orbit.hand(hand);

        for (int index = 0; index < v3d::type::camera::Isometric::AZIMUTHS; ++index) {
            orbit.azimuth(index);
            v3d::type::camera::Camera camera = applied(orbit);
            // the camera draws with the orbit's hand, not whatever the profile was left at
            BOOST_CHECK(camera.profile().hand() == hand);

            const glm::vec3 centre = screen(&camera, orbit.target());
            const glm::vec3 toRight = screen(&camera, orbit.target() + orbit.right() * 2.0F);
            const glm::vec3 away = screen(&camera, orbit.target() + orbit.forward() * 2.0F);

            // a pan built on the wrong hand moves the scene the wrong way while everything
            // else looks correct, so this checks the axes against the screen rather than
            // against a cross product
            BOOST_CHECK_EQUAL(toRight.x > centre.x, true);
            BOOST_CHECK_SMALL(toRight.y - centre.y, 0.01F);

            // away from the eye is up the screen, and clip space points y down
            BOOST_CHECK_EQUAL(away.y < centre.y, true);
            BOOST_CHECK_SMALL(away.x - centre.x, 0.01F);
        }
    }
}

/**
 * The two hands' ground axes are the horizontal mirror of each other: forward() is shared and
 * right() is negated. A pan measured in one hand and drawn through the other moves the wrong
 * way.
 **/
BOOST_AUTO_TEST_CASE(isometriccamera_the_hand_turns_the_pan_axis_test) {
    v3d::type::camera::Isometric orbit;
    v3d::type::camera::Isometric mirrored;
    mirrored.hand(v3d::type::camera::Profile::Hand::DirectionCrossUp);

    for (int index = 0; index < v3d::type::camera::Isometric::AZIMUTHS; ++index) {
        orbit.azimuth(index);
        mirrored.azimuth(index);

        BOOST_CHECK_SMALL(glm::length(mirrored.forward() - orbit.forward()), 0.0001F);
        BOOST_CHECK_SMALL(glm::length(mirrored.right() + orbit.right()), 0.0001F);
    }

    orbit.pan(glm::vec2(3.0F, 0.0F));
    mirrored.pan(glm::vec2(3.0F, 0.0F));
    // and so a pan the same way in both moves the target the other way in world space, which
    // is the same place on screen
    BOOST_CHECK_SMALL(glm::length(mirrored.target() + orbit.target()), 0.0001F);
}

BOOST_AUTO_TEST_CASE(isometriccamera_the_target_is_the_centre_of_the_view_test) {
    v3d::type::camera::Isometric orbit;
    orbit.target(glm::vec3(-6.0F, 0.0F, 2.0F));
    v3d::type::camera::Camera camera = applied(orbit);

    const glm::vec3 centre = screen(&camera, orbit.target());

    BOOST_CHECK_CLOSE(centre.x, static_cast<float>(VIEWPORT_WIDTH) / 2.0F, 0.01F);
    BOOST_CHECK_CLOSE(centre.y, static_cast<float>(VIEWPORT_HEIGHT) / 2.0F, 0.01F);
}

BOOST_AUTO_TEST_CASE(isometriccamera_pan_moves_along_the_view_axes_test) {
    v3d::type::camera::Isometric orbit;
    orbit.azimuth(1);

    const glm::vec3 expected = orbit.right() * 3.0F + orbit.forward() * -2.0F;
    orbit.pan(glm::vec2(3.0F, -2.0F));

    BOOST_CHECK_SMALL(glm::length(orbit.target() - expected), 0.0001F);
    // a pan is across the ground, so it never lifts the target off it
    BOOST_CHECK_SMALL(orbit.target().y, 0.0001F);
}

BOOST_AUTO_TEST_CASE(isometriccamera_zoom_is_clamped_test) {
    v3d::type::camera::Isometric orbit;

    orbit.zoom(12.0F);
    BOOST_CHECK_CLOSE(orbit.zoom(), 12.0F, 0.01F);

    orbit.zoom(v3d::type::camera::Isometric::MINIMUM_ZOOM - 100.0F);
    BOOST_CHECK_CLOSE(orbit.zoom(), v3d::type::camera::Isometric::MINIMUM_ZOOM, 0.01F);

    orbit.zoom(v3d::type::camera::Isometric::MAXIMUM_ZOOM + 100.0F);
    BOOST_CHECK_CLOSE(orbit.zoom(), v3d::type::camera::Isometric::MAXIMUM_ZOOM, 0.01F);

    // the relative form clamps the same way, so holding a key down cannot walk past the end
    orbit.zoom(v3d::type::camera::Isometric::MINIMUM_ZOOM);
    orbit.zoomBy(-5.0F);
    BOOST_CHECK_CLOSE(orbit.zoom(), v3d::type::camera::Isometric::MINIMUM_ZOOM, 0.01F);
    orbit.zoomBy(3.0F);
    BOOST_CHECK_CLOSE(orbit.zoom(), v3d::type::camera::Isometric::MINIMUM_ZOOM + 3.0F, 0.01F);
}

BOOST_AUTO_TEST_CASE(isometriccamera_apply_writes_the_profile_test) {
    v3d::type::camera::Isometric orbit;
    orbit.azimuth(2);
    orbit.zoom(7.0F);
    orbit.target(glm::vec3(1.0F, 0.0F, 5.0F));

    v3d::type::camera::Camera camera;
    camera.orthographic(false);
    camera.profile().clipping(glm::vec2(0.1F, 200.0F));
    orbit.apply(&camera);

    BOOST_CHECK_EQUAL(camera.orthographic(), true);
    BOOST_CHECK_CLOSE(camera.profile().orthoZoom(), 7.0F, 0.01F);
    BOOST_CHECK_SMALL(glm::length(camera.profile().eye() - orbit.eye()), 0.0001F);

    // the clipping distances are the viewport's, not the orbit's, so apply leaves them
    BOOST_CHECK_CLOSE(camera.profile().clipping().x, 0.1F, 0.01F);
    BOOST_CHECK_CLOSE(camera.profile().clipping().y, 200.0F, 0.01F);

    camera.createView();
    const glm::vec4 viewed = camera.view() * glm::vec4(orbit.target(), 1.0F);

    // the target sits on the view axis, one eye distance in front of the camera
    BOOST_CHECK_SMALL(viewed.x, 0.001F);
    BOOST_CHECK_SMALL(viewed.y, 0.001F);
    BOOST_CHECK_CLOSE(viewed.z, orbit.distance(), 0.01F);
}

BOOST_AUTO_TEST_CASE(isometriccamera_apply_ignores_a_null_camera_test) {
    const v3d::type::camera::Isometric orbit;

    orbit.apply(nullptr);
}

};  // namespace

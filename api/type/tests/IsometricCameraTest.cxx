/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/Camera.h>
#include <api/type/IsometricCamera.h>

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
v3d::type::Camera applied(const v3d::type::IsometricCamera& orbit) {
    v3d::type::Camera camera;
    camera.profile().pixelAspect(static_cast<float>(VIEWPORT_WIDTH) / static_cast<float>(VIEWPORT_HEIGHT));
    camera.profile().clipping(0.1f, 200.0f);
    orbit.apply(&camera);
    camera.createProjection();
    camera.createView();
    return camera;
}

glm::vec3 screen(v3d::type::Camera* camera, const glm::vec3& point) {
    int viewport[4] = { 0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT };
    return camera->project(point, viewport);
}

};  // namespace

BOOST_AUTO_TEST_CASE(isometriccamera_defaults_test) {
    const v3d::type::IsometricCamera orbit;

    BOOST_CHECK_EQUAL(orbit.azimuth(), 0);
    BOOST_CHECK_CLOSE(orbit.zoom(), v3d::type::IsometricCamera::DEFAULT_ZOOM, 0.01f);
    BOOST_CHECK_CLOSE(orbit.elevation(), v3d::type::IsometricCamera::DEFAULT_ELEVATION, 0.01f);
    BOOST_CHECK_CLOSE(orbit.distance(), v3d::type::IsometricCamera::DEFAULT_DISTANCE, 0.01f);
    BOOST_CHECK_EQUAL(orbit.target().x, 0.0f);
    BOOST_CHECK_EQUAL(orbit.target().y, 0.0f);
    BOOST_CHECK_EQUAL(orbit.target().z, 0.0f);
}

BOOST_AUTO_TEST_CASE(isometriccamera_rotation_wraps_test) {
    v3d::type::IsometricCamera orbit;

    for (int step = 1; step < v3d::type::IsometricCamera::AZIMUTHS; ++step) {
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
    v3d::type::IsometricCamera orbit;
    orbit.target(glm::vec3(3.0f, 0.0f, -4.0f));

    for (int index = 0; index < v3d::type::IsometricCamera::AZIMUTHS; ++index) {
        orbit.azimuth(index);
        const glm::vec3 offset = orbit.eye() - orbit.target();

        // the eye is one distance from the target whichever corner it is at, and the same
        // height above it - which is what makes the four views comparable
        BOOST_CHECK_CLOSE(glm::length(offset), orbit.distance(), 0.01f);
        BOOST_CHECK_CLOSE(offset.y, std::sin(orbit.elevation()) * orbit.distance(), 0.01f);
    }
}

BOOST_AUTO_TEST_CASE(isometriccamera_axes_are_on_the_ground_test) {
    v3d::type::IsometricCamera orbit;

    for (int index = 0; index < v3d::type::IsometricCamera::AZIMUTHS; ++index) {
        orbit.azimuth(index);

        // both axes lie in the ground plane, are unit length and are at right angles, so a
        // pan cannot lift the target off the plane or scale with the direction it is in
        BOOST_CHECK_SMALL(orbit.right().y, 0.0001f);
        BOOST_CHECK_SMALL(orbit.forward().y, 0.0001f);
        BOOST_CHECK_CLOSE(glm::length(orbit.right()), 1.0f, 0.01f);
        BOOST_CHECK_CLOSE(glm::length(orbit.forward()), 1.0f, 0.01f);
        BOOST_CHECK_SMALL(glm::dot(orbit.right(), orbit.forward()), 0.0001f);
    }
}

BOOST_AUTO_TEST_CASE(isometriccamera_a_quarter_turn_turns_the_axes_test) {
    v3d::type::IsometricCamera orbit;

    const glm::vec3 forward = orbit.forward();
    const glm::vec3 right = orbit.right();

    orbit.rotate(1);
    // a quarter turn counterclockwise takes forward onto where right was pointing back
    BOOST_CHECK_SMALL(glm::length(orbit.forward() + right), 0.0001f);
    BOOST_CHECK_SMALL(glm::length(orbit.right() - forward), 0.0001f);

    orbit.rotate(2);
    // and a half turn from there reverses both
    BOOST_CHECK_SMALL(glm::length(orbit.forward() - right), 0.0001f);
    BOOST_CHECK_SMALL(glm::length(orbit.right() + forward), 0.0001f);
}

BOOST_AUTO_TEST_CASE(isometriccamera_right_is_to_the_right_on_screen_test) {
    v3d::type::IsometricCamera orbit;

    for (int index = 0; index < v3d::type::IsometricCamera::AZIMUTHS; ++index) {
        orbit.azimuth(index);
        v3d::type::Camera camera = applied(orbit);

        const glm::vec3 centre = screen(&camera, orbit.target());
        const glm::vec3 toRight = screen(&camera, orbit.target() + orbit.right() * 2.0f);
        const glm::vec3 away = screen(&camera, orbit.target() + orbit.forward() * 2.0f);

        // which way a camera basis hands is a convention, and a pan built on the other one
        // moves the scene the wrong way with nothing else looking wrong. this is the
        // assertion that says the axes are the screen's and not glm::lookAt's
        BOOST_CHECK_EQUAL(toRight.x > centre.x, true);
        BOOST_CHECK_SMALL(toRight.y - centre.y, 0.01f);

        // away from the eye is up the screen, and clip space points y down - ADR-0012
        BOOST_CHECK_EQUAL(away.y < centre.y, true);
        BOOST_CHECK_SMALL(away.x - centre.x, 0.01f);
    }
}

BOOST_AUTO_TEST_CASE(isometriccamera_the_target_is_the_centre_of_the_view_test) {
    v3d::type::IsometricCamera orbit;
    orbit.target(glm::vec3(-6.0f, 0.0f, 2.0f));
    v3d::type::Camera camera = applied(orbit);

    const glm::vec3 centre = screen(&camera, orbit.target());

    BOOST_CHECK_CLOSE(centre.x, static_cast<float>(VIEWPORT_WIDTH) / 2.0f, 0.01f);
    BOOST_CHECK_CLOSE(centre.y, static_cast<float>(VIEWPORT_HEIGHT) / 2.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(isometriccamera_pan_moves_along_the_view_axes_test) {
    v3d::type::IsometricCamera orbit;
    orbit.azimuth(1);

    const glm::vec3 expected = orbit.right() * 3.0f + orbit.forward() * -2.0f;
    orbit.pan(glm::vec2(3.0f, -2.0f));

    BOOST_CHECK_SMALL(glm::length(orbit.target() - expected), 0.0001f);
    // a pan is across the ground, so it never lifts the target off it
    BOOST_CHECK_SMALL(orbit.target().y, 0.0001f);
}

BOOST_AUTO_TEST_CASE(isometriccamera_zoom_is_clamped_test) {
    v3d::type::IsometricCamera orbit;

    orbit.zoom(12.0f);
    BOOST_CHECK_CLOSE(orbit.zoom(), 12.0f, 0.01f);

    orbit.zoom(v3d::type::IsometricCamera::MINIMUM_ZOOM - 100.0f);
    BOOST_CHECK_CLOSE(orbit.zoom(), v3d::type::IsometricCamera::MINIMUM_ZOOM, 0.01f);

    orbit.zoom(v3d::type::IsometricCamera::MAXIMUM_ZOOM + 100.0f);
    BOOST_CHECK_CLOSE(orbit.zoom(), v3d::type::IsometricCamera::MAXIMUM_ZOOM, 0.01f);

    // the relative form clamps the same way, so holding a key down cannot walk past the end
    orbit.zoom(v3d::type::IsometricCamera::MINIMUM_ZOOM);
    orbit.zoomBy(-5.0f);
    BOOST_CHECK_CLOSE(orbit.zoom(), v3d::type::IsometricCamera::MINIMUM_ZOOM, 0.01f);
    orbit.zoomBy(3.0f);
    BOOST_CHECK_CLOSE(orbit.zoom(), v3d::type::IsometricCamera::MINIMUM_ZOOM + 3.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(isometriccamera_apply_writes_the_profile_test) {
    v3d::type::IsometricCamera orbit;
    orbit.azimuth(2);
    orbit.zoom(7.0f);
    orbit.target(glm::vec3(1.0f, 0.0f, 5.0f));

    v3d::type::Camera camera;
    camera.orthographic(false);
    camera.profile().clipping(0.1f, 200.0f);
    orbit.apply(&camera);

    BOOST_CHECK_EQUAL(camera.orthographic(), true);
    BOOST_CHECK_CLOSE(camera.profile().orthoZoom(), 7.0f, 0.01f);
    BOOST_CHECK_SMALL(glm::length(camera.profile().eye() - orbit.eye()), 0.0001f);

    // the clipping distances are the viewport's, not the orbit's, so apply leaves them
    BOOST_CHECK_CLOSE(camera.profile().clipping().x, 0.1f, 0.01f);
    BOOST_CHECK_CLOSE(camera.profile().clipping().y, 200.0f, 0.01f);

    camera.createView();
    const glm::vec4 viewed = camera.view() * glm::vec4(orbit.target(), 1.0f);

    // the target sits on the view axis, one eye distance in front of the camera
    BOOST_CHECK_SMALL(viewed.x, 0.001f);
    BOOST_CHECK_SMALL(viewed.y, 0.001f);
    BOOST_CHECK_CLOSE(viewed.z, orbit.distance(), 0.01f);
}

BOOST_AUTO_TEST_CASE(isometriccamera_apply_ignores_a_null_camera_test) {
    const v3d::type::IsometricCamera orbit;

    orbit.apply(nullptr);
}

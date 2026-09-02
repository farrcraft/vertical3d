/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <cmath>

#include <boost/test/unit_test.hpp>

#include <glm/gtc/constants.hpp>

#include "../Camera.h"

BOOST_AUTO_TEST_CASE(camera_orthographic_test) {
    v3d::type::Camera camera;

    // a default profile is orthographic
    BOOST_CHECK_EQUAL(camera.orthographic(), true);
    camera.orthographic(false);
    BOOST_CHECK_EQUAL(camera.orthographic(), false);
    camera.orthographic(true);
    BOOST_CHECK_EQUAL(camera.orthographic(), true);
}

BOOST_AUTO_TEST_CASE(camera_projection_test) {
    v3d::type::Camera camera;

    // the orthographic projection spans [-aspect, aspect] horizontally and [-1, 1]
    // vertically, both scaled by the zoom
    camera.createProjection();
    glm::mat4x4 ortho = camera.projection();
    BOOST_CHECK_CLOSE(ortho[0][0], 2.0f / (2.0f * 1.33f), 0.01f);
    BOOST_CHECK_CLOSE(ortho[1][1], 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(ortho[2][2], -2.0f / (100.0f - 0.001f), 0.01f);
    BOOST_CHECK_EQUAL(ortho[3][0], 0.0f);
    BOOST_CHECK_EQUAL(ortho[3][1], 0.0f);
    BOOST_CHECK_EQUAL(ortho[3][3], 1.0f);

    // zooming out widens the volume, so the same world point maps to less of the screen
    camera.zoom(1.0f);
    camera.createProjection();
    glm::mat4x4 zoomed = camera.projection();
    BOOST_CHECK_CLOSE(zoomed[0][0], ortho[0][0] / 2.0f, 0.01f);
    BOOST_CHECK_CLOSE(zoomed[1][1], 0.5f, 0.01f);

    // the perspective projection divides by w, which is where the -1 in the third column
    // and the 0 in the corner come from
    v3d::type::Camera perspective;
    perspective.orthographic(false);
    perspective.createProjection();
    glm::mat4x4 frustum = perspective.projection();
    BOOST_CHECK_CLOSE(frustum[1][1], 1.0f / std::tan(glm::pi<float>() / 6.0f), 0.01f);
    BOOST_CHECK_CLOSE(frustum[0][0], frustum[1][1] / 1.33f, 0.01f);
    BOOST_CHECK_EQUAL(frustum[2][3], -1.0f);
    BOOST_CHECK_EQUAL(frustum[3][3], 0.0f);
}

BOOST_AUTO_TEST_CASE(camera_view_test) {
    v3d::type::Camera camera;

    // the view matrix translates the world by the negated eye position, so a default
    // camera sitting at z = -1 pushes the world one unit the other way
    camera.createView();
    BOOST_CHECK_EQUAL(camera.view()[3][0], 0.0f);
    BOOST_CHECK_EQUAL(camera.view()[3][1], 0.0f);
    BOOST_CHECK_EQUAL(camera.view()[3][2], 1.0f);

    // dolly moves the eye along the direction of view
    camera.dolly(2.0f);
    camera.createView();
    BOOST_CHECK_CLOSE(camera.view()[3][2], -1.0f, 0.01f);

    // truck moves it along the right vector, pedestal along the up vector
    camera.truck(3.0f);
    camera.pedestal(4.0f);
    camera.createView();
    BOOST_CHECK_CLOSE(camera.view()[3][0], -3.0f, 0.01f);
    BOOST_CHECK_CLOSE(camera.view()[3][1], -4.0f, 0.01f);

    // an explicit eye position replaces whatever the moves accumulated
    camera.profile().eye(glm::vec3(0.0f, 0.0f, -1.0f));
    camera.createView();
    BOOST_CHECK_EQUAL(camera.view()[3][2], 1.0f);
}

BOOST_AUTO_TEST_CASE(camera_project_test) {
    v3d::type::Camera camera;
    camera.createProjection();
    camera.createView();

    int viewport[4] = { 0, 0, 640, 480 };

    // the centre of the near volume lands in the middle of the viewport
    glm::vec3 centre = camera.project(glm::vec3(0.0f, 0.0f, 0.0f), viewport);
    BOOST_CHECK_CLOSE(centre[0], 320.0f, 0.01f);
    BOOST_CHECK_CLOSE(centre[1], 240.0f, 0.01f);

    // and unproject takes a screen point back to where it came from. Only points on the
    // horizontal centre line round trip: project measures y downward from the top of the
    // viewport and unproject measures it upward from the bottom.
    glm::vec3 world(0.5f, 0.0f, 0.25f);
    glm::vec3 screen = camera.project(world, viewport);
    glm::vec3 roundTrip = camera.unproject(screen, viewport);
    BOOST_CHECK_CLOSE(roundTrip[0], world[0], 0.1f);
    BOOST_CHECK_SMALL(roundTrip[1], 0.001f);
    BOOST_CHECK_CLOSE(roundTrip[2], world[2], 0.1f);
}

BOOST_AUTO_TEST_CASE(camera_ortho_factor_test) {
    v3d::type::Camera camera;

    // nothing has given the camera a viewport, so there is nothing to divide by
    BOOST_CHECK_EQUAL(camera.orthoFactorHorizontal(), 0.0f);
    BOOST_CHECK_EQUAL(camera.orthoFactorVertical(), 0.0f);

    camera.profile().size(640, 480);

    // the horizontal factor carries the pixel aspect ratio and the vertical one does not
    BOOST_CHECK_CLOSE(camera.orthoFactorHorizontal(), (1.0f * 2.0f * 1.33f) / 640.0f, 0.01f);
    BOOST_CHECK_CLOSE(camera.orthoFactorVertical(), (1.0f * 2.0f) / 480.0f, 0.01f);

    // zooming out covers more world per pixel in both directions
    camera.zoom(1.0f);
    BOOST_CHECK_CLOSE(camera.orthoFactorHorizontal(), (2.0f * 2.0f * 1.33f) / 640.0f, 0.01f);
    BOOST_CHECK_CLOSE(camera.orthoFactorVertical(), (2.0f * 2.0f) / 480.0f, 0.01f);
}

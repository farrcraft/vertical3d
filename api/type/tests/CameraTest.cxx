/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/Camera.h>
#include <api/type/Ray.h>

#include <cmath>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

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
    // vertically, both scaled by the zoom. The vertical scale is negative because vulkan
    // clip space points y downward where the camera's axes point it up - ADR-0012
    camera.createProjection();
    glm::mat4x4 ortho = camera.projection();
    BOOST_CHECK_CLOSE(ortho[0][0], 2.0f / (2.0f * 1.33f), 0.01f);
    BOOST_CHECK_CLOSE(ortho[1][1], -1.0f, 0.01f);
    // depth runs from zero at the near plane to one at the far one
    BOOST_CHECK_CLOSE(ortho[2][2], 1.0f / (100.0f - 0.001f), 0.01f);
    BOOST_CHECK_EQUAL(ortho[3][0], 0.0f);
    BOOST_CHECK_EQUAL(ortho[3][1], 0.0f);
    BOOST_CHECK_EQUAL(ortho[3][3], 1.0f);

    // zooming out widens the volume, so the same world point maps to less of the screen
    camera.zoom(1.0f);
    camera.createProjection();
    glm::mat4x4 zoomed = camera.projection();
    BOOST_CHECK_CLOSE(zoomed[0][0], ortho[0][0] / 2.0f, 0.01f);
    BOOST_CHECK_CLOSE(zoomed[1][1], -0.5f, 0.01f);

    // the perspective projection divides by w, which is where the -1 in the third column
    // and the 0 in the corner come from
    v3d::type::Camera perspective;
    perspective.orthographic(false);
    perspective.createProjection();
    glm::mat4x4 frustum = perspective.projection();
    BOOST_CHECK_CLOSE(frustum[1][1], -1.0f / std::tan(glm::pi<float>() / 6.0f), 0.01f);
    BOOST_CHECK_CLOSE(frustum[0][0], -frustum[1][1] / 1.33f, 0.01f);
    // w is the view z rather than its negation - the camera looks along its own +z
    BOOST_CHECK_EQUAL(frustum[2][3], 1.0f);
    BOOST_CHECK_EQUAL(frustum[3][3], 0.0f);
}

BOOST_AUTO_TEST_CASE(camera_depth_range_test) {
    // a point on the near plane lands at depth zero and one on the far plane at depth one,
    // which is the range vulkan clips against and what the engine clears depth to. The
    // camera looks along +z, so both points are in front of it
    v3d::type::Camera perspective;
    perspective.orthographic(false);
    perspective.profile().clipping(1.0f, 100.0f);
    perspective.profile().eye(glm::vec3(0.0f, 0.0f, 0.0f));
    perspective.createProjection();
    perspective.createView();

    glm::vec4 near = perspective.projection() * perspective.view() * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    BOOST_CHECK_SMALL(near[2] / near[3], 0.001f);

    glm::vec4 far = perspective.projection() * perspective.view() * glm::vec4(0.0f, 0.0f, 100.0f, 1.0f);
    BOOST_CHECK_CLOSE(far[2] / far[3], 1.0f, 0.01f);

    v3d::type::Camera ortho;
    ortho.profile().clipping(1.0f, 100.0f);
    ortho.profile().eye(glm::vec3(0.0f, 0.0f, 0.0f));
    ortho.createProjection();
    ortho.createView();

    glm::vec4 orthoNear = ortho.projection() * ortho.view() * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    BOOST_CHECK_SMALL(orthoNear[2], 0.001f);
    glm::vec4 orthoFar = ortho.projection() * ortho.view() * glm::vec4(0.0f, 0.0f, 100.0f, 1.0f);
    BOOST_CHECK_CLOSE(orthoFar[2], 1.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(camera_lookat_test) {
    // the rotation a lookat writes takes the camera into the basis its normals define, and
    // createView transposes it back - so a camera told to look at a point sees that point
    // straight ahead, on its own +z axis and on neither of the other two
    v3d::type::Camera camera;
    camera.profile().eye(glm::vec3(0.0f, 10.0f, 0.0f));
    camera.profile().up(glm::vec3(0.0f, 0.0f, 1.0f));
    camera.profile().lookat(glm::vec3(0.0f, 0.0f, 0.0f));
    camera.createView();

    glm::vec4 origin = camera.view() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    BOOST_CHECK_SMALL(origin[0], 0.001f);
    BOOST_CHECK_SMALL(origin[1], 0.001f);
    BOOST_CHECK_CLOSE(origin[2], 10.0f, 0.01f);

    // and the normals it derived are the ones a top view has
    BOOST_CHECK_CLOSE(camera.profile().direction()[1], -1.0f, 0.01f);
    BOOST_CHECK_CLOSE(camera.profile().right()[0], 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(camera.profile().up()[2], 1.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(camera_perspective_view_test) {
    // a perspective camera's view matrix is built the same way an orthographic one's is:
    // translate by the negated eye, then rotate into the camera's axes. Doing it the other
    // way round rotates the eye offset along with the world
    v3d::type::Camera camera;
    camera.orthographic(false);
    camera.profile().eye(glm::vec3(3.0f, 4.0f, 5.0f));
    camera.profile().rotation(glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3(0.0f, 1.0f, 0.0f)));
    camera.createView();

    // the eye itself is the origin of view space whatever the rotation is
    glm::vec4 eye = camera.view() * glm::vec4(3.0f, 4.0f, 5.0f, 1.0f);
    BOOST_CHECK_SMALL(eye[0], 0.001f);
    BOOST_CHECK_SMALL(eye[1], 0.001f);
    BOOST_CHECK_SMALL(eye[2], 0.001f);
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

    // and unproject takes a screen point back to where it came from. Both measure y
    // downward from the top of the viewport, so a point off the centre line round trips too
    glm::vec3 world(0.5f, 0.25f, 1.0f);
    glm::vec3 screen = camera.project(world, viewport);
    glm::vec3 roundTrip = camera.unproject(screen, viewport);
    BOOST_CHECK_CLOSE(roundTrip[0], world[0], 0.1f);
    BOOST_CHECK_CLOSE(roundTrip[1], world[1], 0.1f);
    BOOST_CHECK_CLOSE(roundTrip[2], world[2], 0.1f);
}

BOOST_AUTO_TEST_CASE(camera_ray_test) {
    v3d::type::Camera camera;
    camera.createProjection();
    camera.createView();

    int viewport[4] = { 0, 0, 640, 480 };

    // an orthographic camera casts a ray parallel to its direction of view from wherever
    // the click was, so an off centre click does not tilt it
    v3d::type::Ray centre = camera.ray(glm::vec2(320.0f, 240.0f), viewport);
    v3d::type::Ray corner = camera.ray(glm::vec2(0.0f, 0.0f), viewport);
    BOOST_CHECK_CLOSE(centre.direction()[2], 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(corner.direction()[2], 1.0f, 0.01f);
    BOOST_CHECK_LT(corner.origin()[0], centre.origin()[0]);

    // the ray runs back through the point that was clicked: a world point projected to the
    // screen and cast back sits on the ray it came from
    glm::vec3 world(0.5f, 0.25f, 5.0f);
    glm::vec3 screen = camera.project(world, viewport);
    v3d::type::Ray back = camera.ray(glm::vec2(screen[0], screen[1]), viewport);
    glm::vec3 along = back.point(glm::length(world - back.origin()));
    BOOST_CHECK_CLOSE(along[0], world[0], 0.1f);
    BOOST_CHECK_CLOSE(along[1], world[1], 0.1f);
    BOOST_CHECK_CLOSE(along[2], world[2], 0.1f);

    // a perspective camera fans its rays out from the eye instead
    v3d::type::Camera perspective;
    perspective.orthographic(false);
    perspective.createProjection();
    perspective.createView();
    v3d::type::Ray middle = perspective.ray(glm::vec2(320.0f, 240.0f), viewport);
    v3d::type::Ray edge = perspective.ray(glm::vec2(0.0f, 240.0f), viewport);
    BOOST_CHECK_SMALL(middle.direction()[0], 0.001f);
    BOOST_CHECK_LT(edge.direction()[0], -0.1f);
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

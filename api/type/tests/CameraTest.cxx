/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/camera/Camera.h>
#include <api/type/geometry/Plane.h>
#include <api/type/geometry/Ray.h>

#include <cmath>
#include <optional>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat3x3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {

BOOST_AUTO_TEST_CASE(camera_orthographic_test) {
    v3d::type::camera::Camera camera;

    // a default profile is orthographic
    BOOST_CHECK_EQUAL(camera.orthographic(), true);
    camera.orthographic(false);
    BOOST_CHECK_EQUAL(camera.orthographic(), false);
    camera.orthographic(true);
    BOOST_CHECK_EQUAL(camera.orthographic(), true);
}

BOOST_AUTO_TEST_CASE(camera_projection_test) {
    v3d::type::camera::Camera camera;

    // the orthographic projection spans [-aspect, aspect] horizontally and [-1, 1]
    // vertically, both scaled by the zoom. The vertical scale is negative because vulkan
    // clip space points y downward where the camera's axes point it up
    camera.createProjection();
    glm::mat4x4 ortho = camera.projection();
    BOOST_CHECK_CLOSE(ortho[0][0], 2.0F / (2.0F * 1.33F), 0.01F);
    BOOST_CHECK_CLOSE(ortho[1][1], -1.0F, 0.01F);
    // depth runs from zero at the near plane to one at the far one
    BOOST_CHECK_CLOSE(ortho[2][2], 1.0F / (100.0F - 0.001F), 0.01F);
    BOOST_CHECK_EQUAL(ortho[3][0], 0.0F);
    BOOST_CHECK_EQUAL(ortho[3][1], 0.0F);
    BOOST_CHECK_EQUAL(ortho[3][3], 1.0F);

    // zooming out widens the volume, so the same world point maps to less of the screen
    camera.zoom(1.0F);
    camera.createProjection();
    glm::mat4x4 zoomed = camera.projection();
    BOOST_CHECK_CLOSE(zoomed[0][0], ortho[0][0] / 2.0F, 0.01F);
    BOOST_CHECK_CLOSE(zoomed[1][1], -0.5F, 0.01F);

    // the perspective projection divides by w, which is where the -1 in the third column
    // and the 0 in the corner come from
    v3d::type::camera::Camera perspective;
    perspective.orthographic(false);
    perspective.createProjection();
    glm::mat4x4 frustum = perspective.projection();
    BOOST_CHECK_CLOSE(frustum[1][1], -1.0F / std::tan(glm::pi<float>() / 6.0F), 0.01F);
    BOOST_CHECK_CLOSE(frustum[0][0], -frustum[1][1] / 1.33F, 0.01F);
    // w is the view z rather than its negation - the camera looks along its own +z
    BOOST_CHECK_EQUAL(frustum[2][3], 1.0F);
    BOOST_CHECK_EQUAL(frustum[3][3], 0.0F);
}

/**
 * One large zoom in leaves the half height at its floor rather than at zero or below, so the
 * next drag, which is scaled by the half height, can still zoom back out.
 **/
BOOST_AUTO_TEST_CASE(camera_zoom_has_a_floor_test) {
    v3d::type::camera::Camera camera;
    camera.zoom(-50.0F);
    BOOST_CHECK_CLOSE(camera.profile().orthoZoom(), v3d::type::camera::Camera::MINIMUM_ZOOM, 0.01F);

    camera.zoom(1.0F);
    BOOST_CHECK_CLOSE(camera.profile().orthoZoom(), v3d::type::camera::Camera::MINIMUM_ZOOM + 1.0F, 0.01F);
}

BOOST_AUTO_TEST_CASE(camera_depth_range_test) {
    // a point on the near plane lands at depth zero and one on the far plane at depth one,
    // which is the range vulkan clips against and what the engine clears depth to. The
    // camera looks along +z, so both points are in front of it
    v3d::type::camera::Camera perspective;
    perspective.orthographic(false);
    perspective.profile().clipping(glm::vec2(1.0F, 100.0F));
    perspective.profile().eye(glm::vec3(0.0F, 0.0F, 0.0F));
    perspective.createProjection();
    perspective.createView();

    glm::vec4 near = perspective.projection() * perspective.view() * glm::vec4(0.0F, 0.0F, 1.0F, 1.0F);
    BOOST_CHECK_SMALL(near[2] / near[3], 0.001F);

    glm::vec4 far = perspective.projection() * perspective.view() * glm::vec4(0.0F, 0.0F, 100.0F, 1.0F);
    BOOST_CHECK_CLOSE(far[2] / far[3], 1.0F, 0.01F);

    v3d::type::camera::Camera ortho;
    ortho.profile().clipping(glm::vec2(1.0F, 100.0F));
    ortho.profile().eye(glm::vec3(0.0F, 0.0F, 0.0F));
    ortho.createProjection();
    ortho.createView();

    glm::vec4 orthoNear = ortho.projection() * ortho.view() * glm::vec4(0.0F, 0.0F, 1.0F, 1.0F);
    BOOST_CHECK_SMALL(orthoNear[2], 0.001F);
    glm::vec4 orthoFar = ortho.projection() * ortho.view() * glm::vec4(0.0F, 0.0F, 100.0F, 1.0F);
    BOOST_CHECK_CLOSE(orthoFar[2], 1.0F, 0.01F);
}

BOOST_AUTO_TEST_CASE(camera_lookat_test) {
    // the rotation a lookat writes takes the camera into the basis its normals define, and
    // createView transposes it back - so a camera told to look at a point sees that point
    // straight ahead, on its own +z axis and on neither of the other two
    v3d::type::camera::Camera camera;
    camera.profile().eye(glm::vec3(0.0F, 10.0F, 0.0F));
    camera.profile().up(glm::vec3(0.0F, 0.0F, 1.0F));
    camera.profile().lookat(glm::vec3(0.0F, 0.0F, 0.0F));
    camera.createView();

    glm::vec4 origin = camera.view() * glm::vec4(0.0F, 0.0F, 0.0F, 1.0F);
    BOOST_CHECK_SMALL(origin[0], 0.001F);
    BOOST_CHECK_SMALL(origin[1], 0.001F);
    BOOST_CHECK_CLOSE(origin[2], 10.0F, 0.01F);

    // and the normals it derived are the ones a top view has
    BOOST_CHECK_CLOSE(camera.profile().direction()[1], -1.0F, 0.01F);
    BOOST_CHECK_CLOSE(camera.profile().right()[0], 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(camera.profile().up()[2], 1.0F, 0.01F);
}

/**
 * The two hands mirror each other horizontally and agree about which way is up. The default
 * is UpCrossDirection. DirectionCrossUp is the basis glm::lookAt builds from the same eye, up
 * and centre, so an application whose geometry was wound for glm can use this camera.
 **/
BOOST_AUTO_TEST_CASE(camera_profile_hand_test) {
    v3d::type::camera::Profile profile("top");
    BOOST_CHECK(profile.hand() == v3d::type::camera::Profile::Hand::UpCrossDirection);

    profile.eye(glm::vec3(0.0F, 10.0F, 0.0F));
    profile.up(glm::vec3(0.0F, 0.0F, 1.0F));
    profile.lookat(glm::vec3(0.0F, 0.0F, 0.0F));

    // right = up x direction, as the rest of this suite asserts
    BOOST_CHECK_CLOSE(profile.right()[0], 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(profile.up()[2], 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(profile.direction()[1], -1.0F, 0.01F);

    v3d::type::camera::Profile mirrored("top");
    mirrored.hand(v3d::type::camera::Profile::Hand::DirectionCrossUp);
    mirrored.eye(glm::vec3(0.0F, 10.0F, 0.0F));
    mirrored.up(glm::vec3(0.0F, 0.0F, 1.0F));
    mirrored.lookat(glm::vec3(0.0F, 0.0F, 0.0F));

    // the same direction and the same up, and the right negated. That is the only
    // difference, and it reverses the winding of a front face
    BOOST_CHECK_CLOSE(mirrored.right()[0], -1.0F, 0.01F);
    BOOST_CHECK_CLOSE(mirrored.up()[2], 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(mirrored.direction()[1], -1.0F, 0.01F);

    // the hand travels with the profile, so a clone keeps building the basis it was built for
    v3d::type::camera::Profile copied("copy");
    copied.clone(mirrored);
    BOOST_CHECK(copied.hand() == v3d::type::camera::Profile::Hand::DirectionCrossUp);
}

/**
 * The two hands mirror each other through the view matrix: the same world point lands at the
 * same height and the same depth in both, and at the negated x.
 *
 * The normals alone do not show this. lookat() writes them directly from the cross products,
 * so they are correct whatever the rotation holds. No quaternion represents the mirrored
 * basis, so the rotation holds the unmirrored basis and createView() applies the mirror. This
 * test also checks that the rotation is a proper rotation: a quat_cast of a mirror gives
 * columns that are not unit length.
 **/
BOOST_AUTO_TEST_CASE(camera_hands_build_mirrored_views_test) {
    const glm::vec3 eye(6.0F, 8.0F, 10.0F);
    const glm::vec3 centre(1.0F, 0.0F, -2.0F);

    v3d::type::camera::Camera camera;
    camera.profile().eye(eye);
    camera.profile().up(glm::vec3(0.0F, 1.0F, 0.0F));
    camera.profile().lookat(centre);
    camera.createView();

    v3d::type::camera::Camera mirrored;
    mirrored.profile().hand(v3d::type::camera::Profile::Hand::DirectionCrossUp);
    mirrored.profile().eye(eye);
    mirrored.profile().up(glm::vec3(0.0F, 1.0F, 0.0F));
    mirrored.profile().lookat(centre);
    mirrored.createView();

    // both rotations are proper rotations, with unit columns
    const glm::mat3 basis(glm::mat3_cast(camera.profile().rotation()));
    const glm::mat3 mirroredBasis(glm::mat3_cast(mirrored.profile().rotation()));
    BOOST_CHECK_CLOSE(glm::determinant(basis), 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(glm::determinant(mirroredBasis), 1.0F, 0.01F);
    for (int column = 0; column < 3; column++) {
        BOOST_CHECK_CLOSE(glm::length(mirroredBasis[column]), 1.0F, 0.01F);
    }

    // and the two views are the same picture reflected: points off both axes and off the
    // centre, so a view that merely happened to agree about one of them does not pass
    const glm::vec3 points[] = {
        glm::vec3(0.0F, 0.0F, 0.0F),
        glm::vec3(3.0F, 1.0F, -4.0F),
        glm::vec3(-5.0F, 2.0F, 7.0F),
        glm::vec3(1.0F, -6.0F, 2.0F)
    };
    for (const glm::vec3& point : points) {
        const glm::vec4 through = camera.view() * glm::vec4(point, 1.0F);
        const glm::vec4 reflected = mirrored.view() * glm::vec4(point, 1.0F);
        BOOST_CHECK_CLOSE(reflected[0], -through[0], 0.01F);
        BOOST_CHECK_CLOSE(reflected[1], through[1], 0.01F);
        BOOST_CHECK_CLOSE(reflected[2], through[2], 0.01F);
    }

    // the eye is still the origin of view space in the mirrored basis, which holds only
    // while the view is a rigid transform
    const glm::vec4 origin = mirrored.view() * glm::vec4(eye, 1.0F);
    BOOST_CHECK_SMALL(origin[0], 0.001F);
    BOOST_CHECK_SMALL(origin[1], 0.001F);
    BOOST_CHECK_SMALL(origin[2], 0.001F);
}

BOOST_AUTO_TEST_CASE(camera_perspective_view_test) {
    // a perspective camera's view matrix is built the same way an orthographic one's is:
    // translate by the negated eye, then rotate into the camera's axes. Doing it the other
    // way round rotates the eye offset along with the world
    v3d::type::camera::Camera camera;
    camera.orthographic(false);
    camera.profile().eye(glm::vec3(3.0F, 4.0F, 5.0F));
    camera.profile().rotation(glm::angleAxis(glm::pi<float>() / 2.0F, glm::vec3(0.0F, 1.0F, 0.0F)));
    camera.createView();

    // the eye itself is the origin of view space whatever the rotation is
    glm::vec4 eye = camera.view() * glm::vec4(3.0F, 4.0F, 5.0F, 1.0F);
    BOOST_CHECK_SMALL(eye[0], 0.001F);
    BOOST_CHECK_SMALL(eye[1], 0.001F);
    BOOST_CHECK_SMALL(eye[2], 0.001F);
}

BOOST_AUTO_TEST_CASE(camera_view_test) {
    v3d::type::camera::Camera camera;

    // the view matrix translates the world by the negated eye position, so a default
    // camera sitting at z = -1 pushes the world one unit the other way
    camera.createView();
    BOOST_CHECK_EQUAL(camera.view()[3][0], 0.0F);
    BOOST_CHECK_EQUAL(camera.view()[3][1], 0.0F);
    BOOST_CHECK_EQUAL(camera.view()[3][2], 1.0F);

    // dolly moves the eye along the direction of view
    camera.dolly(2.0F);
    camera.createView();
    BOOST_CHECK_CLOSE(camera.view()[3][2], -1.0F, 0.01F);

    // truck moves it along the right vector, pedestal along the up vector
    camera.truck(3.0F);
    camera.pedestal(4.0F);
    camera.createView();
    BOOST_CHECK_CLOSE(camera.view()[3][0], -3.0F, 0.01F);
    BOOST_CHECK_CLOSE(camera.view()[3][1], -4.0F, 0.01F);

    // an explicit eye position replaces whatever the moves accumulated
    camera.profile().eye(glm::vec3(0.0F, 0.0F, -1.0F));
    camera.createView();
    BOOST_CHECK_EQUAL(camera.view()[3][2], 1.0F);
}

BOOST_AUTO_TEST_CASE(camera_project_test) {
    v3d::type::camera::Camera camera;
    camera.createProjection();
    camera.createView();

    int viewport[4] = { 0, 0, 640, 480 };

    // the centre of the near volume lands in the middle of the viewport
    glm::vec3 centre = camera.project(glm::vec3(0.0F, 0.0F, 0.0F), viewport);
    BOOST_CHECK_CLOSE(centre[0], 320.0F, 0.01F);
    BOOST_CHECK_CLOSE(centre[1], 240.0F, 0.01F);

    // and unproject takes a screen point back to where it came from. Both measure y
    // downward from the top of the viewport, so a point off the centre line round trips too
    glm::vec3 world(0.5F, 0.25F, 1.0F);
    const glm::vec3 screen = camera.project(world, viewport);
    glm::vec3 roundTrip = camera.unproject(screen, viewport);
    BOOST_CHECK_CLOSE(roundTrip[0], world[0], 0.1F);
    BOOST_CHECK_CLOSE(roundTrip[1], world[1], 0.1F);
    BOOST_CHECK_CLOSE(roundTrip[2], world[2], 0.1F);
}

BOOST_AUTO_TEST_CASE(camera_ray_test) {
    v3d::type::camera::Camera camera;
    camera.createProjection();
    camera.createView();

    int viewport[4] = { 0, 0, 640, 480 };

    // an orthographic camera casts a ray parallel to its direction of view from wherever
    // the click was, so an off centre click does not tilt it
    const v3d::type::geometry::Ray centre = camera.ray(glm::vec2(320.0F, 240.0F), viewport);
    const v3d::type::geometry::Ray corner = camera.ray(glm::vec2(0.0F, 0.0F), viewport);
    BOOST_CHECK_CLOSE(centre.direction()[2], 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(corner.direction()[2], 1.0F, 0.01F);
    BOOST_CHECK_LT(corner.origin()[0], centre.origin()[0]);

    // the ray runs back through the point that was clicked: a world point projected to the
    // screen and cast back sits on the ray it came from
    glm::vec3 world(0.5F, 0.25F, 5.0F);
    glm::vec3 screen = camera.project(world, viewport);
    const v3d::type::geometry::Ray back = camera.ray(glm::vec2(screen[0], screen[1]), viewport);
    glm::vec3 along = back.point(glm::length(world - back.origin()));
    BOOST_CHECK_CLOSE(along[0], world[0], 0.1F);
    BOOST_CHECK_CLOSE(along[1], world[1], 0.1F);
    BOOST_CHECK_CLOSE(along[2], world[2], 0.1F);

    // a perspective camera fans its rays out from the eye instead
    v3d::type::camera::Camera perspective;
    perspective.orthographic(false);
    perspective.createProjection();
    perspective.createView();
    const v3d::type::geometry::Ray middle = perspective.ray(glm::vec2(320.0F, 240.0F), viewport);
    const v3d::type::geometry::Ray edge = perspective.ray(glm::vec2(0.0F, 240.0F), viewport);
    BOOST_CHECK_SMALL(middle.direction()[0], 0.001F);
    BOOST_CHECK_LT(edge.direction()[0], -0.1F);
}

/**
 * A point behind a perspective camera has no screen point, and one in front lands where
 * project() puts it. Under an orthographic camera w is always one and nothing is behind it, so
 * the case uses a perspective one.
 **/
BOOST_AUTO_TEST_CASE(camera_screen_point_test) {
    v3d::type::camera::Camera perspective;
    perspective.orthographic(false);
    perspective.createProjection();
    perspective.createView();
    int viewport[4] = { 0, 0, 640, 480 };

    // the default camera looks along +z from the origin
    const std::optional<glm::vec3> ahead = perspective.screenPoint(glm::vec3(0.0F, 0.0F, 5.0F), viewport);
    BOOST_REQUIRE(ahead.has_value());
    BOOST_CHECK_CLOSE((*ahead)[0], 320.0F, 0.01F);
    BOOST_CHECK_CLOSE((*ahead)[1], 240.0F, 0.01F);

    BOOST_CHECK(!perspective.screenPoint(glm::vec3(0.0F, 0.0F, -5.0F), viewport).has_value());
    BOOST_CHECK(!perspective.screenPoint(glm::vec3(1.0F, 1.0F, -5.0F), viewport).has_value());

    int empty[4] = { 0, 0, 0, 480 };
    BOOST_CHECK(!perspective.screenPoint(glm::vec3(0.0F, 0.0F, 5.0F), empty).has_value());
}

/**
 * The cursor at the middle of the viewport picks the point on the ground straight below a camera
 * looking down, and a cursor whose ray never meets the ground picks nothing.
 **/
BOOST_AUTO_TEST_CASE(camera_pick_test) {
    v3d::type::camera::Camera camera;
    camera.profile().eye(glm::vec3(2.0F, 10.0F, 3.0F));
    camera.profile().up(glm::vec3(0.0F, 0.0F, 1.0F));
    camera.profile().lookat(glm::vec3(2.0F, 0.0F, 3.0F));
    camera.createProjection();
    camera.createView();
    int viewport[4] = { 0, 0, 640, 480 };

    v3d::type::geometry::Plane ground;
    ground.calculate(glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(0.0F));
    const std::optional<glm::vec3> picked = camera.pick(glm::vec2(320.0F, 240.0F), viewport, ground);
    BOOST_REQUIRE(picked.has_value());
    BOOST_CHECK_CLOSE((*picked)[0], 2.0F, 0.1F);
    BOOST_CHECK_SMALL((*picked)[1], 0.001F);
    BOOST_CHECK_CLOSE((*picked)[2], 3.0F, 0.1F);

    // a plane above the camera is behind every ray it casts downward
    v3d::type::geometry::Plane ceiling;
    ceiling.calculate(glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(0.0F, 20.0F, 0.0F));
    BOOST_CHECK(!camera.pick(glm::vec2(320.0F, 240.0F), viewport, ceiling).has_value());
}

BOOST_AUTO_TEST_CASE(camera_ortho_factor_test) {
    v3d::type::camera::Camera camera;

    // nothing has given the camera a viewport, so there is nothing to divide by
    BOOST_CHECK_EQUAL(camera.orthoFactorHorizontal(), 0.0F);
    BOOST_CHECK_EQUAL(camera.orthoFactorVertical(), 0.0F);

    camera.profile().size(glm::uvec2(640, 480));

    // the horizontal factor carries the pixel aspect ratio and the vertical one does not
    BOOST_CHECK_CLOSE(camera.orthoFactorHorizontal(), (1.0F * 2.0F * 1.33F) / 640.0F, 0.01F);
    BOOST_CHECK_CLOSE(camera.orthoFactorVertical(), (1.0F * 2.0F) / 480.0F, 0.01F);

    // zooming out covers more world per pixel in both directions
    camera.zoom(1.0F);
    BOOST_CHECK_CLOSE(camera.orthoFactorHorizontal(), (2.0F * 2.0F * 1.33F) / 640.0F, 0.01F);
    BOOST_CHECK_CLOSE(camera.orthoFactorVertical(), (2.0F * 2.0F) / 480.0F, 0.01F);
}

/**
 * A view built through lookat() equals the one glm::lookAt builds, element for element, with
 * no tolerance. It is checked exactly because a consumer that compares rendered images
 * against stored references sees a difference of one unit in the last place.
 *
 * Two conventions are undone before the views are compared. This camera looks along +z where
 * glm looks along -z, so row 2 is negated. glm's basis is the DirectionCrossUp hand, so the
 * profile uses that hand. Equality is checked
 * with == rather than by comparing bits, because the mirror turns some zeros negative and
 * -0.0f == 0.0f while their bits differ.
 **/
BOOST_AUTO_TEST_CASE(camera_lookat_matches_glm_exactly_test) {
    const glm::vec3 eyes[] = {
        glm::vec3(10.0F, 10.0F, 10.0F),
        glm::vec3(-10.0F, 10.0F, 10.0F),
        glm::vec3(-10.0F, 10.0F, -10.0F),
        glm::vec3(10.0F, 10.0F, -10.0F),
        glm::vec3(3.5F, 2.25F, -7.125F),
        glm::vec3(1234.5F, -67.125F, 0.03125F)
    };
    const glm::vec3 centres[] = {
        glm::vec3(0.0F, 0.0F, 0.0F),
        glm::vec3(0.0F, 0.0F, 0.0F),
        glm::vec3(0.0F, 0.0F, 0.0F),
        glm::vec3(0.0F, 0.0F, 0.0F),
        glm::vec3(1.5F, 0.5F, 2.0F),
        glm::vec3(-9.5F, 4.0F, 11.0F)
    };

    for (unsigned int which = 0; which < 6; ++which) {
        const glm::vec3 up(0.0F, 1.0F, 0.0F);

        v3d::type::camera::Camera camera;
        camera.profile().hand(v3d::type::camera::Profile::Hand::DirectionCrossUp);
        camera.profile().eye(eyes[which]);
        camera.profile().up(up);
        camera.profile().lookat(centres[which]);
        camera.createView();

        glm::mat4x4 expected = glm::lookAt(eyes[which], centres[which], up);
        for (int column = 0; column < 4; ++column) {
            expected[column][2] = -expected[column][2];
        }

        const glm::mat4x4 built = camera.view();
        for (int column = 0; column < 4; ++column) {
            for (int row = 0; row < 4; ++row) {
                BOOST_CHECK_EQUAL(built[column][row], expected[column][row]);
            }
        }
    }
}

/**
 * A rotation set directly still builds its own view. The cached basis describes only the
 * rotation lookat() built, so every other write of the rotation must clear it.
 **/
BOOST_AUTO_TEST_CASE(camera_a_set_rotation_outlives_a_cached_basis_test) {
    v3d::type::camera::Camera camera;
    camera.profile().eye(glm::vec3(0.0F, 0.0F, 5.0F));
    camera.profile().lookat(glm::vec3(0.0F, 0.0F, 0.0F));
    camera.createView();

    // a quarter turn about y, set rather than looked at, is the view that must be built
    const glm::quat turned = glm::angleAxis(glm::pi<float>() / 2.0F, glm::vec3(0.0F, 1.0F, 0.0F));
    camera.profile().rotation(turned);
    camera.createView();

    glm::mat4x4 cast = glm::transpose(glm::mat4_cast(turned));
    cast = glm::translate(cast, -camera.profile().eye());
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            BOOST_CHECK_EQUAL(camera.view()[column][row], cast[column][row]);
        }
    }

    // and panning after a lookat() is the same: the basis it cached is not this rotation
    v3d::type::camera::Camera panned;
    panned.profile().eye(glm::vec3(0.0F, 0.0F, 5.0F));
    panned.profile().lookat(glm::vec3(0.0F, 0.0F, 0.0F));
    panned.pan(glm::pi<float>() / 4.0F);
    panned.createView();

    glm::mat4x4 expected = glm::transpose(glm::mat4_cast(panned.profile().rotation()));
    expected = glm::translate(expected, -panned.profile().eye());
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            BOOST_CHECK_EQUAL(panned.view()[column][row], expected[column][row]);
        }
    }
}

BOOST_AUTO_TEST_CASE(camera_rotate_after_a_lookat_turns_the_view_test) {
    // the editor's perspective camera is a lookat() profile that the arcball then rotates
    v3d::type::camera::Camera camera;
    camera.profile().eye(glm::vec3(0.0F, 0.0F, 5.0F));
    camera.profile().lookat(glm::vec3(0.0F, 0.0F, 0.0F));
    camera.createView();
    const glm::mat4x4 before = camera.view();

    // a turn about one axis, which has two zero components
    const glm::quat turn = glm::angleAxis(glm::pi<float>() / 6.0F, glm::vec3(0.0F, 1.0F, 0.0F));
    camera.rotate(turn);
    camera.createView();

    glm::mat4x4 expected = glm::transpose(glm::mat4_cast(camera.profile().rotation()));
    expected = glm::translate(expected, -camera.profile().eye());
    float moved = 0.0F;
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            BOOST_CHECK_SMALL(camera.view()[column][row] - expected[column][row], 0.0001F);
            moved += std::fabs(camera.view()[column][row] - before[column][row]);
        }
    }
    BOOST_CHECK_GT(moved, 0.1F);
}

BOOST_AUTO_TEST_CASE(camera_a_degenerate_rotate_is_no_rotation_test) {
    v3d::type::camera::Camera camera;
    camera.profile().eye(glm::vec3(0.0F, 0.0F, 5.0F));
    camera.profile().lookat(glm::vec3(0.0F, 0.0F, 0.0F));
    const glm::quat before = camera.profile().rotation();

    // what ArcBall::drag returns when the cursor has not moved
    camera.rotate(glm::quat(0.0F, 0.0F, 0.0F, 0.0F));
    BOOST_CHECK(camera.profile().rotation() == before);
}

BOOST_AUTO_TEST_CASE(camera_a_turn_moves_the_normals_with_it_test) {
    v3d::type::camera::Camera camera;
    camera.profile().eye(glm::vec3(0.0F, 0.0F, -5.0F));
    camera.profile().lookat(glm::vec3(0.0F, 0.0F, 0.0F));
    BOOST_CHECK_CLOSE(camera.profile().direction()[2], 1.0F, 0.01F);

    // a quarter pan about the camera's own y takes the view direction onto world x, and a
    // dolly after it has to follow
    camera.pan(glm::half_pi<float>());
    const glm::vec3 direction = camera.profile().direction();
    BOOST_CHECK_SMALL(direction[2], 0.0001F);
    BOOST_CHECK_CLOSE(std::fabs(direction[0]), 1.0F, 0.01F);

    const glm::vec3 eye = camera.profile().eye();
    camera.dolly(2.0F);
    BOOST_CHECK_SMALL(glm::length(camera.profile().eye() - (eye + direction * 2.0F)), 0.0001F);

    // and the normals are the view's: direction is the third row of the view's rotation
    camera.createView();
    const glm::vec3 viewZ(camera.view()[0][2], camera.view()[1][2], camera.view()[2][2]);
    BOOST_CHECK_SMALL(glm::length(viewZ - direction), 0.0001F);
}

};  // namespace

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/camera/Camera.h>
#include <api/type/geometry/Ray.h>

#include <cmath>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat3x3.hpp>
#include <glm/gtc/quaternion.hpp>

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
    v3d::type::camera::Camera perspective;
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
    v3d::type::camera::Camera perspective;
    perspective.orthographic(false);
    perspective.profile().clipping(1.0f, 100.0f);
    perspective.profile().eye(glm::vec3(0.0f, 0.0f, 0.0f));
    perspective.createProjection();
    perspective.createView();

    glm::vec4 near = perspective.projection() * perspective.view() * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    BOOST_CHECK_SMALL(near[2] / near[3], 0.001f);

    glm::vec4 far = perspective.projection() * perspective.view() * glm::vec4(0.0f, 0.0f, 100.0f, 1.0f);
    BOOST_CHECK_CLOSE(far[2] / far[3], 1.0f, 0.01f);

    v3d::type::camera::Camera ortho;
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
    v3d::type::camera::Camera camera;
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

/**
 * The two hands mirror each other horizontally and agree about which way is up. The default
 * is what every profile in this tree has always meant, and the other one is the basis
 * glm::lookAt builds from the same eye, up and centre - so an application whose geometry was
 * wound for that one can be handed this camera instead of writing a second.
 **/
BOOST_AUTO_TEST_CASE(camera_profile_hand_test) {
    v3d::type::camera::Profile profile("top");
    BOOST_CHECK(profile.hand() == v3d::type::camera::Profile::Hand::UpCrossDirection);

    profile.eye(glm::vec3(0.0f, 10.0f, 0.0f));
    profile.up(glm::vec3(0.0f, 0.0f, 1.0f));
    profile.lookat(glm::vec3(0.0f, 0.0f, 0.0f));

    // right = up x direction, which is what the rest of this suite asserts
    BOOST_CHECK_CLOSE(profile.right()[0], 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.up()[2], 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.direction()[1], -1.0f, 0.01f);

    v3d::type::camera::Profile mirrored("top");
    mirrored.hand(v3d::type::camera::Profile::Hand::DirectionCrossUp);
    mirrored.eye(glm::vec3(0.0f, 10.0f, 0.0f));
    mirrored.up(glm::vec3(0.0f, 0.0f, 1.0f));
    mirrored.lookat(glm::vec3(0.0f, 0.0f, 0.0f));

    // the same direction and the same up, and the right the other way round - the whole of
    // the difference, and the reason the winding a front face presents reverses with it
    BOOST_CHECK_CLOSE(mirrored.right()[0], -1.0f, 0.01f);
    BOOST_CHECK_CLOSE(mirrored.up()[2], 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(mirrored.direction()[1], -1.0f, 0.01f);

    // the hand travels with the profile, so a clone keeps building the basis it was built for
    v3d::type::camera::Profile copied("copy");
    copied.clone(mirrored);
    BOOST_CHECK(copied.hand() == v3d::type::camera::Profile::Hand::DirectionCrossUp);
}

/**
 * The two hands are mirrors of each other through the matrix a caller draws with, which is
 * the thing that makes one of them usable rather than merely different: the same world point
 * lands at the same height and the same depth in both, and at the negated x.
 *
 * Asserting the normals is not enough and was the gap that shipped. lookat() writes those
 * from the cross products directly, so they are right whatever the rotation carries, and the
 * mirrored basis is improper - no quaternion represents it. The rotation is the right handed
 * half and createView() applies the mirror, so this also checks the rotation is still a
 * rotation: a quat_cast of a mirror comes back with columns that are not unit length.
 **/
BOOST_AUTO_TEST_CASE(camera_hands_build_mirrored_views_test) {
    const glm::vec3 eye(6.0f, 8.0f, 10.0f);
    const glm::vec3 centre(1.0f, 0.0f, -2.0f);

    v3d::type::camera::Camera camera;
    camera.profile().eye(eye);
    camera.profile().up(glm::vec3(0.0f, 1.0f, 0.0f));
    camera.profile().lookat(centre);
    camera.createView();

    v3d::type::camera::Camera mirrored;
    mirrored.profile().hand(v3d::type::camera::Profile::Hand::DirectionCrossUp);
    mirrored.profile().eye(eye);
    mirrored.profile().up(glm::vec3(0.0f, 1.0f, 0.0f));
    mirrored.profile().lookat(centre);
    mirrored.createView();

    // both rotations are rotations, which is what the mirrored one was not while lookat()
    // built it out of an improper basis
    const glm::mat3 basis(glm::mat3_cast(camera.profile().rotation()));
    const glm::mat3 mirroredBasis(glm::mat3_cast(mirrored.profile().rotation()));
    BOOST_CHECK_CLOSE(glm::determinant(basis), 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(glm::determinant(mirroredBasis), 1.0f, 0.01f);
    for (int column = 0; column < 3; column++) {
        BOOST_CHECK_CLOSE(glm::length(mirroredBasis[column]), 1.0f, 0.01f);
    }

    // and the two views are the same picture reflected: points off both axes and off the
    // centre, so a view that merely happened to agree about one of them does not pass
    const glm::vec3 points[] = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(3.0f, 1.0f, -4.0f),
        glm::vec3(-5.0f, 2.0f, 7.0f),
        glm::vec3(1.0f, -6.0f, 2.0f)
    };
    for (const glm::vec3& point : points) {
        const glm::vec4 through = camera.view() * glm::vec4(point, 1.0f);
        const glm::vec4 reflected = mirrored.view() * glm::vec4(point, 1.0f);
        BOOST_CHECK_CLOSE(reflected[0], -through[0], 0.01f);
        BOOST_CHECK_CLOSE(reflected[1], through[1], 0.01f);
        BOOST_CHECK_CLOSE(reflected[2], through[2], 0.01f);
    }

    // the eye is still the origin of view space in the mirrored basis - a mirror through the
    // rotation moved it, because what came back was not a rigid transform
    const glm::vec4 origin = mirrored.view() * glm::vec4(eye, 1.0f);
    BOOST_CHECK_SMALL(origin[0], 0.001f);
    BOOST_CHECK_SMALL(origin[1], 0.001f);
    BOOST_CHECK_SMALL(origin[2], 0.001f);
}

BOOST_AUTO_TEST_CASE(camera_perspective_view_test) {
    // a perspective camera's view matrix is built the same way an orthographic one's is:
    // translate by the negated eye, then rotate into the camera's axes. Doing it the other
    // way round rotates the eye offset along with the world
    v3d::type::camera::Camera camera;
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
    v3d::type::camera::Camera camera;

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
    v3d::type::camera::Camera camera;
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
    v3d::type::camera::Camera camera;
    camera.createProjection();
    camera.createView();

    int viewport[4] = { 0, 0, 640, 480 };

    // an orthographic camera casts a ray parallel to its direction of view from wherever
    // the click was, so an off centre click does not tilt it
    v3d::type::geometry::Ray centre = camera.ray(glm::vec2(320.0f, 240.0f), viewport);
    v3d::type::geometry::Ray corner = camera.ray(glm::vec2(0.0f, 0.0f), viewport);
    BOOST_CHECK_CLOSE(centre.direction()[2], 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(corner.direction()[2], 1.0f, 0.01f);
    BOOST_CHECK_LT(corner.origin()[0], centre.origin()[0]);

    // the ray runs back through the point that was clicked: a world point projected to the
    // screen and cast back sits on the ray it came from
    glm::vec3 world(0.5f, 0.25f, 5.0f);
    glm::vec3 screen = camera.project(world, viewport);
    v3d::type::geometry::Ray back = camera.ray(glm::vec2(screen[0], screen[1]), viewport);
    glm::vec3 along = back.point(glm::length(world - back.origin()));
    BOOST_CHECK_CLOSE(along[0], world[0], 0.1f);
    BOOST_CHECK_CLOSE(along[1], world[1], 0.1f);
    BOOST_CHECK_CLOSE(along[2], world[2], 0.1f);

    // a perspective camera fans its rays out from the eye instead
    v3d::type::camera::Camera perspective;
    perspective.orthographic(false);
    perspective.createProjection();
    perspective.createView();
    v3d::type::geometry::Ray middle = perspective.ray(glm::vec2(320.0f, 240.0f), viewport);
    v3d::type::geometry::Ray edge = perspective.ray(glm::vec2(0.0f, 240.0f), viewport);
    BOOST_CHECK_SMALL(middle.direction()[0], 0.001f);
    BOOST_CHECK_LT(edge.direction()[0], -0.1f);
}

BOOST_AUTO_TEST_CASE(camera_ortho_factor_test) {
    v3d::type::camera::Camera camera;

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

/**
 * A view built through lookat() is the one glm::lookAt builds, element for element and with
 * no tolerance at all.
 *
 * **This is the assertion the cached basis exists for**, and it is why it is asserted exactly:
 * a consumer holding its own reference frames re-baselines them for a difference of one unit
 * in the last place, so "close enough" is the thing that costs rather than the thing that
 * passes. Reported by retcon as U18, whose capture moved 153 of 891600 pixels when it adopted
 * this camera.
 *
 * Two conventions have to be undone before the two are comparable, neither of them a
 * difference in the arithmetic: this tree looks along +z where glm looks along -z, so row 2
 * is negated, and ADR-0052's mirrored hand is the basis glm crosses for. Equality is checked
 * with == rather than by comparing bits, because the mirror turns some zeros negative and
 * -0.0f == 0.0f while their bits differ.
 **/
BOOST_AUTO_TEST_CASE(camera_lookat_matches_glm_exactly_test) {
    const glm::vec3 eyes[] = {
        glm::vec3(10.0f, 10.0f, 10.0f),
        glm::vec3(-10.0f, 10.0f, 10.0f),
        glm::vec3(-10.0f, 10.0f, -10.0f),
        glm::vec3(10.0f, 10.0f, -10.0f),
        glm::vec3(3.5f, 2.25f, -7.125f),
        glm::vec3(1234.5f, -67.125f, 0.03125f)
    };
    const glm::vec3 centres[] = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.5f, 0.5f, 2.0f),
        glm::vec3(-9.5f, 4.0f, 11.0f)
    };

    for (unsigned int which = 0; which < 6; ++which) {
        const glm::vec3 up(0.0f, 1.0f, 0.0f);

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
 * A rotation set directly still builds its own view, which is the half a cached basis can
 * break: the matrix lookat() kept describes the rotation lookat() built, and any other writer
 * of the rotation has to put it back to being cast from the quaternion.
 **/
BOOST_AUTO_TEST_CASE(camera_a_set_rotation_outlives_a_cached_basis_test) {
    v3d::type::camera::Camera camera;
    camera.profile().eye(glm::vec3(0.0f, 0.0f, 5.0f));
    camera.profile().lookat(glm::vec3(0.0f, 0.0f, 0.0f));
    camera.createView();

    // a quarter turn about y, set rather than looked at, is the view that has to win
    const glm::quat turned = glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
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
    panned.profile().eye(glm::vec3(0.0f, 0.0f, 5.0f));
    panned.profile().lookat(glm::vec3(0.0f, 0.0f, 0.0f));
    panned.pan(glm::pi<float>() / 4.0f);
    panned.createView();

    glm::mat4x4 expected = glm::transpose(glm::mat4_cast(panned.profile().rotation()));
    expected = glm::translate(expected, -panned.profile().eye());
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            BOOST_CHECK_EQUAL(panned.view()[column][row], expected[column][row]);
        }
    }
}

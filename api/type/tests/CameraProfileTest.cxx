/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../Camera.h"
#include "../CameraProfile.h"

/**
 * A profile keeps its state protected and hands Camera the only key, so everything a test
 * can observe about it has to be observed through a camera built on it.
 **/
BOOST_AUTO_TEST_CASE(cameraprofile_test) {
    v3d::type::CameraProfile profile("test");
    profile.eye(glm::vec3(4.0f, 5.0f, 6.0f));

    // a camera constructed from a profile takes a copy of it
    v3d::type::Camera camera(profile);
    camera.createView();
    BOOST_CHECK_CLOSE(camera.view()[3][0], -4.0f, 0.01f);
    BOOST_CHECK_CLOSE(camera.view()[3][1], -5.0f, 0.01f);
    BOOST_CHECK_CLOSE(camera.view()[3][2], -6.0f, 0.01f);

    // moving the camera does not reach back into the profile it was built from
    camera.truck(10.0f);
    camera.createView();
    BOOST_CHECK_CLOSE(camera.view()[3][0], -14.0f, 0.01f);
    v3d::type::Camera untouched(profile);
    untouched.createView();
    BOOST_CHECK_CLOSE(untouched.view()[3][0], -4.0f, 0.01f);

    // clipping planes reach the projection
    profile.clipping(1.0f, 3.0f);
    v3d::type::Camera clipped(profile);
    clipped.createProjection();
    BOOST_CHECK_CLOSE(clipped.projection()[2][2], -2.0f / (3.0f - 1.0f), 0.01f);
    BOOST_CHECK_CLOSE(clipped.projection()[3][2], -(3.0f + 1.0f) / (3.0f - 1.0f), 0.01f);

    // assignment copies every field, so a camera on the copy sees the same view
    v3d::type::CameraProfile duplicate("duplicate");
    duplicate = profile;
    v3d::type::Camera copied(duplicate);
    copied.createView();
    BOOST_CHECK_CLOSE(copied.view()[3][0], -4.0f, 0.01f);
    BOOST_CHECK_CLOSE(copied.view()[3][1], -5.0f, 0.01f);
    BOOST_CHECK_CLOSE(copied.view()[3][2], -6.0f, 0.01f);

    // lookat orients the camera at a target: looking down -z from the origin leaves the
    // rotation alone, so the view matrix stays a pure translation
    v3d::type::CameraProfile facing("facing");
    facing.eye(glm::vec3(0.0f, 0.0f, 0.0f));
    facing.lookat(glm::vec3(0.0f, 0.0f, 1.0f));
    v3d::type::Camera looking(facing);
    looking.createView();
    BOOST_CHECK_CLOSE(looking.view()[0][0], 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(looking.view()[1][1], 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(looking.view()[2][2], 1.0f, 0.01f);
}

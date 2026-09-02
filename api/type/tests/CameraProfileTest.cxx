/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../Camera.h"
#include "../CameraProfile.h"

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

BOOST_AUTO_TEST_CASE(cameraprofile_accessor_test) {
    v3d::type::CameraProfile profile("test");

    // the defaults, which are what a partly described profile falls back to
    BOOST_CHECK_EQUAL(profile.name(), "test");
    BOOST_CHECK_CLOSE(profile.clipping()[0], 0.001f, 0.01f);
    BOOST_CHECK_CLOSE(profile.clipping()[1], 100.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.fov(), 60.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.orthoZoom(), 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.pixelAspect(), 1.33f, 0.01f);
    BOOST_CHECK_EQUAL(profile.orthographic(), true);
    BOOST_CHECK_EQUAL(profile.adaptiveProjection(), false);
    BOOST_CHECK_EQUAL(profile.adaptivePosition(), false);
    BOOST_CHECK_EQUAL(profile.size()[0], 0u);
    BOOST_CHECK_EQUAL(profile.size()[1], 0u);

    profile.name("front");
    profile.clipping(0.5f, 50.0f);
    profile.fov(45.0f);
    profile.orthoZoom(10.0f);
    profile.pixelAspect(1.6f);
    profile.orthographic(false);
    profile.adaptiveProjection(true);
    profile.adaptivePosition(true);
    profile.eye(glm::vec3(1.0f, 2.0f, 3.0f));
    profile.up(glm::vec3(0.0f, 0.0f, 1.0f));
    profile.right(glm::vec3(1.0f, 0.0f, 0.0f));
    profile.direction(glm::vec3(0.0f, -1.0f, 0.0f));
    profile.rotation(glm::quat(0.5f, 0.5f, 0.5f, 0.5f));
    profile.size(1024, 768);

    BOOST_CHECK_EQUAL(profile.name(), "front");
    BOOST_CHECK_CLOSE(profile.clipping()[0], 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(profile.clipping()[1], 50.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.fov(), 45.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.orthoZoom(), 10.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.pixelAspect(), 1.6f, 0.01f);
    BOOST_CHECK_EQUAL(profile.orthographic(), false);
    BOOST_CHECK_EQUAL(profile.adaptiveProjection(), true);
    BOOST_CHECK_EQUAL(profile.adaptivePosition(), true);
    BOOST_CHECK_CLOSE(profile.eye()[1], 2.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.up()[2], 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.right()[0], 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.direction()[1], -1.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.rotation().w, 0.5f, 0.01f);
    BOOST_CHECK_EQUAL(profile.size()[0], 1024u);
    BOOST_CHECK_EQUAL(profile.size()[1], 768u);

    // the three option bits are independent - clearing one leaves the others standing
    profile.adaptiveProjection(false);
    BOOST_CHECK_EQUAL(profile.adaptiveProjection(), false);
    BOOST_CHECK_EQUAL(profile.adaptivePosition(), true);
    BOOST_CHECK_EQUAL(profile.orthographic(), false);

    // clone copies the viewport size along with everything else, which the ortho factors
    // divide by
    v3d::type::CameraProfile assigned("assigned");
    assigned = profile;
    BOOST_CHECK_EQUAL(assigned.name(), "front");
    BOOST_CHECK_EQUAL(assigned.size()[0], 1024u);
    BOOST_CHECK_EQUAL(assigned.adaptivePosition(), true);
}

BOOST_AUTO_TEST_CASE(cameraprofile_basis_test) {
    // name plus the four basis vectors, with every other field left at its default
    v3d::type::CameraProfile profile("Top",
        glm::vec3(0.0f, 10.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, -1.0f, 0.0f));

    BOOST_CHECK_EQUAL(profile.name(), "Top");
    BOOST_CHECK_CLOSE(profile.eye()[1], 10.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.up()[2], 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.right()[0], 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.direction()[1], -1.0f, 0.01f);

    // lookat writes all four together, so it overwrites the basis the constructor was given
    profile.lookat(glm::vec3(0.0f, 0.0f, 0.0f));
    BOOST_CHECK_CLOSE(profile.direction()[1], -1.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.right()[0], 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(profile.up()[2], 1.0f, 0.01f);
    BOOST_CHECK_SMALL(profile.up()[1], 0.001f);
}

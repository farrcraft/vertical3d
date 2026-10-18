/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>

#include <boost/test/unit_test.hpp>

namespace {

BOOST_AUTO_TEST_CASE(cameraprofile_test) {
    v3d::type::camera::Profile profile("test");
    profile.eye(glm::vec3(4.0F, 5.0F, 6.0F));

    // a camera constructed from a profile takes a copy of it
    v3d::type::camera::Camera camera(profile);
    camera.createView();
    BOOST_CHECK_CLOSE(camera.view()[3][0], -4.0F, 0.01F);
    BOOST_CHECK_CLOSE(camera.view()[3][1], -5.0F, 0.01F);
    BOOST_CHECK_CLOSE(camera.view()[3][2], -6.0F, 0.01F);

    // moving the camera does not reach back into the profile it was built from
    camera.truck(10.0F);
    camera.createView();
    BOOST_CHECK_CLOSE(camera.view()[3][0], -14.0F, 0.01F);
    v3d::type::camera::Camera untouched(profile);
    untouched.createView();
    BOOST_CHECK_CLOSE(untouched.view()[3][0], -4.0F, 0.01F);

    // clipping planes reach the projection, which maps them onto the [0, 1] depth range
    // vulkan clips against
    profile.clipping(glm::vec2(1.0F, 3.0F));
    v3d::type::camera::Camera clipped(profile);
    clipped.createProjection();
    BOOST_CHECK_CLOSE(clipped.projection()[2][2], 1.0F / (3.0F - 1.0F), 0.01F);
    BOOST_CHECK_CLOSE(clipped.projection()[3][2], -1.0F / (3.0F - 1.0F), 0.01F);

    // assignment copies every field, so a camera on the copy sees the same view
    v3d::type::camera::Profile duplicate("duplicate");
    duplicate = profile;
    v3d::type::camera::Camera copied(duplicate);
    copied.createView();
    BOOST_CHECK_CLOSE(copied.view()[3][0], -4.0F, 0.01F);
    BOOST_CHECK_CLOSE(copied.view()[3][1], -5.0F, 0.01F);
    BOOST_CHECK_CLOSE(copied.view()[3][2], -6.0F, 0.01F);

    // lookat orients the camera at a target: looking along +z from the origin leaves the
    // rotation alone, so the view matrix stays a pure translation
    v3d::type::camera::Profile facing("facing");
    facing.eye(glm::vec3(0.0F, 0.0F, 0.0F));
    facing.lookat(glm::vec3(0.0F, 0.0F, 1.0F));
    v3d::type::camera::Camera looking(facing);
    looking.createView();
    BOOST_CHECK_CLOSE(looking.view()[0][0], 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(looking.view()[1][1], 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(looking.view()[2][2], 1.0F, 0.01F);
}

BOOST_AUTO_TEST_CASE(cameraprofile_accessor_test) {
    v3d::type::camera::Profile profile("test");

    // the defaults, which are what a partly described profile falls back to
    BOOST_CHECK_EQUAL(profile.name(), "test");
    BOOST_CHECK_CLOSE(profile.clipping()[0], 0.001F, 0.01F);
    BOOST_CHECK_CLOSE(profile.clipping()[1], 100.0F, 0.01F);
    BOOST_CHECK_CLOSE(profile.fov(), 60.0F, 0.01F);
    BOOST_CHECK_CLOSE(profile.orthoZoom(), 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(profile.pixelAspect(), 1.33F, 0.01F);
    BOOST_CHECK_EQUAL(profile.orthographic(), true);
    BOOST_CHECK_EQUAL(profile.adaptiveProjection(), false);
    BOOST_CHECK_EQUAL(profile.adaptivePosition(), false);
    BOOST_CHECK_EQUAL(profile.size()[0], 0U);
    BOOST_CHECK_EQUAL(profile.size()[1], 0U);

    profile.name("front");
    profile.clipping(glm::vec2(0.5F, 50.0F));
    profile.fov(45.0F);
    profile.orthoZoom(10.0F);
    profile.pixelAspect(1.6F);
    profile.orthographic(false);
    profile.adaptiveProjection(true);
    profile.adaptivePosition(true);
    profile.eye(glm::vec3(1.0F, 2.0F, 3.0F));
    profile.up(glm::vec3(0.0F, 0.0F, 1.0F));
    profile.right(glm::vec3(1.0F, 0.0F, 0.0F));
    profile.direction(glm::vec3(0.0F, -1.0F, 0.0F));
    profile.rotation(glm::quat(0.5F, 0.5F, 0.5F, 0.5F));
    profile.size(glm::uvec2(1024, 768));

    BOOST_CHECK_EQUAL(profile.name(), "front");
    BOOST_CHECK_CLOSE(profile.clipping()[0], 0.5F, 0.01F);
    BOOST_CHECK_CLOSE(profile.clipping()[1], 50.0F, 0.01F);
    BOOST_CHECK_CLOSE(profile.fov(), 45.0F, 0.01F);
    BOOST_CHECK_CLOSE(profile.orthoZoom(), 10.0F, 0.01F);
    BOOST_CHECK_CLOSE(profile.pixelAspect(), 1.6F, 0.01F);
    BOOST_CHECK_EQUAL(profile.orthographic(), false);
    BOOST_CHECK_EQUAL(profile.adaptiveProjection(), true);
    BOOST_CHECK_EQUAL(profile.adaptivePosition(), true);
    BOOST_CHECK_CLOSE(profile.eye()[1], 2.0F, 0.01F);
    BOOST_CHECK_CLOSE(profile.up()[2], 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(profile.right()[0], 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(profile.direction()[1], -1.0F, 0.01F);
    BOOST_CHECK_CLOSE(profile.rotation().w, 0.5F, 0.01F);
    BOOST_CHECK_EQUAL(profile.size()[0], 1024U);
    BOOST_CHECK_EQUAL(profile.size()[1], 768U);

    // the three option bits are independent - clearing one leaves the others standing
    profile.adaptiveProjection(false);
    BOOST_CHECK_EQUAL(profile.adaptiveProjection(), false);
    BOOST_CHECK_EQUAL(profile.adaptivePosition(), true);
    BOOST_CHECK_EQUAL(profile.orthographic(), false);

    // clone copies the viewport size along with everything else, which the ortho factors
    // divide by
    v3d::type::camera::Profile assigned("assigned");
    assigned = profile;
    BOOST_CHECK_EQUAL(assigned.name(), "front");
    BOOST_CHECK_EQUAL(assigned.size()[0], 1024U);
    BOOST_CHECK_EQUAL(assigned.adaptivePosition(), true);
}

BOOST_AUTO_TEST_CASE(cameraprofile_basis_test) {
    // name plus the four basis vectors, with every other field left at its default
    v3d::type::camera::Profile profile("Top", {
        .eye = glm::vec3(0.0F, 10.0F, 0.0F),
        .up = glm::vec3(0.0F, 0.0F, 1.0F),
        .right = glm::vec3(1.0F, 0.0F, 0.0F),
        .direction = glm::vec3(0.0F, -1.0F, 0.0F)});

    BOOST_CHECK_EQUAL(profile.name(), "Top");
    BOOST_CHECK_CLOSE(profile.eye()[1], 10.0F, 0.01F);
    BOOST_CHECK_CLOSE(profile.up()[2], 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(profile.right()[0], 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(profile.direction()[1], -1.0F, 0.01F);

    // lookat writes all four together, so it overwrites the basis the constructor was given
    profile.lookat(glm::vec3(0.0F, 0.0F, 0.0F));
    BOOST_CHECK_CLOSE(profile.direction()[1], -1.0F, 0.01F);
    BOOST_CHECK_CLOSE(profile.right()[0], 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(profile.up()[2], 1.0F, 0.01F);
    BOOST_CHECK_SMALL(profile.up()[1], 0.001F);
}

};  // namespace

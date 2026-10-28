/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/camera/Camera.h>
#include <demos/voxel/src/engine/CameraController.h>

#include <cmath>

#include <boost/test/unit_test.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {

void checkClose(const glm::vec3 & actual, const glm::vec3 & expected) {
    BOOST_TEST_CONTEXT("actual (" << actual.x << ", " << actual.y << ", " << actual.z << ")") {
        BOOST_CHECK_SMALL(glm::length(actual - expected), 0.0001F);
    }
}

/** A controller with no motion, over a camera at the origin looking down -z with +y up. **/
struct Fixture {
    v3d::type::camera::Camera camera;
    CameraController controller{&camera, CameraController::Motion()};
};

/**
 * A positive heading turns the view to the right, about the world's up axis, by that many
 * degrees. Up stays up.
 **/
BOOST_FIXTURE_TEST_CASE(camera_a_heading_turns_right_test, Fixture) {
    controller.look(90.0F, 0.0F);
    checkClose(camera.profile().direction(), glm::vec3(1.0F, 0.0F, 0.0F));
    checkClose(camera.profile().up(), glm::vec3(0.0F, 1.0F, 0.0F));
    checkClose(camera.profile().right(), glm::vec3(0.0F, 0.0F, 1.0F));
}

/**
 * A positive pitch turns the view down, and the view stops at straight down.
 **/
BOOST_FIXTURE_TEST_CASE(camera_pitch_stops_straight_down_test, Fixture) {
    controller.look(0.0F, 30.0F);
    checkClose(camera.profile().direction(), glm::vec3(0.0F, -0.5F, -std::sqrt(3.0F) / 2.0F));

    controller.look(0.0F, 100.0F);
    checkClose(camera.profile().direction(), glm::vec3(0.0F, -1.0F, 0.0F));
}

/**
 * A heading and a pitch in one turn pitch the view once and then turn it about the world's up
 * axis. A view pitched down 30 degrees and turned right 90 looks along +x, 30 degrees down.
 **/
BOOST_FIXTURE_TEST_CASE(camera_heading_and_pitch_together_test, Fixture) {
    controller.look(90.0F, 30.0F);
    checkClose(camera.profile().direction(), glm::vec3(std::sqrt(3.0F) / 2.0F, -0.5F, 0.0F));
}

/**
 * The projection keeps the horizontal field of view it is given at any aspect. A point 45
 * degrees to the right reaches the right edge of clip space under a 90 degree field of view,
 * and the vertical scale is the horizontal one times the aspect.
 **/
BOOST_FIXTURE_TEST_CASE(camera_perspective_keeps_its_horizontal_field_of_view_test, Fixture) {
    for (const float aspect : { 1.0F, 2.0F }) {
        BOOST_TEST_CONTEXT("aspect " << aspect) {
            controller.perspective({.fovx = 90.0F, .aspect = aspect, .znear = 0.1F, .zfar = 100.0F});
            const glm::vec4 clip = camera.projection() * camera.view() * glm::vec4(1.0F, 0.0F, -1.0F, 1.0F);
            BOOST_CHECK_CLOSE(clip.x / clip.w, 1.0F, 0.01F);
            // y is negated, because vulkan's clip space has y pointing down
            const glm::vec4 above = camera.projection() * camera.view() * glm::vec4(0.0F, 1.0F, -aspect, 1.0F);
            BOOST_CHECK_CLOSE(above.y / above.w, -1.0F, 0.01F);
        }
    }
}

/**
 * The camera puts a world point where glm::lookAt and a horizontal field of view put it, which
 * is the space voxel's terrain is wound for. A point right of the eye lands right of centre, a
 * point above lands above it, and depth runs from zero at the near plane to one at the far one.
 **/
BOOST_FIXTURE_TEST_CASE(camera_matches_a_look_at_view_test, Fixture) {
    const glm::vec3 eye(3.0F, 4.0F, 5.0F);
    controller.place(eye);
    controller.look(30.0F, 20.0F);
    controller.perspective({.fovx = 90.0F, .aspect = 1.5F, .znear = 0.1F, .zfar = 100.0F});

    const glm::mat4 lookAt = glm::lookAt(eye, eye + camera.profile().direction(), glm::vec3(0.0F, 1.0F, 0.0F));
    for (const glm::vec3 offset : { glm::vec3(1.0F, 0.5F, -4.0F), glm::vec3(-2.0F, -1.0F, -7.0F), glm::vec3(0.0F, 0.0F, -0.1F) }) {
        // the offset is in glm::lookAt's view space: x right, y up, and -z ahead
        const glm::vec3 point = eye + glm::vec3(glm::inverse(lookAt) * glm::vec4(offset, 0.0F));
        const glm::vec4 clip = camera.projection() * camera.view() * glm::vec4(point, 1.0F);
        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        BOOST_TEST_CONTEXT("offset (" << offset.x << ", " << offset.y << ", " << offset.z << ")") {
            // the horizontal scale of a 90 degree field is one, and the vertical one the aspect
            const float distance = -offset.z;
            BOOST_CHECK_SMALL(ndc.x - (offset.x / distance), 0.0001F);
            BOOST_CHECK_SMALL(ndc.y - (-offset.y * 1.5F / distance), 0.0001F);
            BOOST_CHECK_SMALL(ndc.z - ((100.0F / (100.0F - 0.1F)) * (1.0F - (0.1F / distance))), 0.0001F);
        }
    }
}

/**
 * A move forward goes along the ground whatever the pitch, so looking down does not slow a
 * walk, and a move up goes up the world's axis.
 **/
BOOST_FIXTURE_TEST_CASE(camera_moves_along_the_ground_test, Fixture) {
    controller.look(90.0F, 60.0F);
    checkClose(controller.moved(glm::vec3(0.0F, 0.0F, 2.0F)), glm::vec3(2.0F, 0.0F, 0.0F));
    checkClose(controller.moved(glm::vec3(1.0F, 3.0F, 0.0F)), glm::vec3(0.0F, 3.0F, 1.0F));
}

};  // namespace

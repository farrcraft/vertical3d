/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <demos/voxel/src/engine/Camera.h>

#include <cmath>

#include <boost/test/unit_test.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace {

/** A first person camera at the origin, looking down -z with +y up. **/
Camera firstPerson() {
    Camera camera;
    camera.behavior(Camera::CAMERA_BEHAVIOR_FIRST_PERSON);
    return camera;
}

void checkClose(const glm::vec3 & actual, const glm::vec3 & expected) {
    BOOST_TEST_CONTEXT("actual (" << actual.x << ", " << actual.y << ", " << actual.z << ")") {
        BOOST_CHECK_SMALL(glm::length(actual - expected), 0.0001F);
    }
}

/**
 * A positive heading turns the view to the right, about the world's up axis, by that many
 * degrees. Up stays up.
 **/
BOOST_AUTO_TEST_CASE(camera_a_heading_turns_right_test) {
    Camera camera = firstPerson();
    camera.rotate(90.0F, 0.0F, 0.0F);
    checkClose(camera.direction(), glm::vec3(1.0F, 0.0F, 0.0F));
    checkClose(camera.yAxis(), glm::vec3(0.0F, 1.0F, 0.0F));
}

/**
 * A positive pitch turns the view down, and a first person view stops at straight down.
 **/
BOOST_AUTO_TEST_CASE(camera_pitch_stops_straight_down_test) {
    Camera camera = firstPerson();
    camera.rotate(0.0F, 30.0F, 0.0F);
    checkClose(camera.direction(), glm::vec3(0.0F, -0.5F, -std::sqrt(3.0F) / 2.0F));

    camera.rotate(0.0F, 100.0F, 0.0F);
    checkClose(camera.direction(), glm::vec3(0.0F, -1.0F, 0.0F));
}

/**
 * A heading and a pitch in one turn pitch the view once and then turn it about the world's up
 * axis. A view pitched down 30 degrees and turned right 90 looks along +x, 30 degrees down.
 **/
BOOST_AUTO_TEST_CASE(camera_heading_and_pitch_together_test) {
    Camera camera = firstPerson();
    camera.rotate(90.0F, 30.0F, 0.0F);
    checkClose(camera.direction(), glm::vec3(std::sqrt(3.0F) / 2.0F, -0.5F, 0.0F));
}

/**
 * A flight camera rolls about its own view axis. A positive roll banks to the right, so the
 * right axis tips down.
 **/
BOOST_AUTO_TEST_CASE(camera_a_roll_banks_right_test) {
    Camera camera;
    camera.behavior(Camera::CAMERA_BEHAVIOR_FLIGHT);
    camera.rotate(0.0F, 0.0F, 90.0F);
    checkClose(camera.xAxis(), glm::vec3(0.0F, -1.0F, 0.0F));
    checkClose(camera.yAxis(), glm::vec3(1.0F, 0.0F, 0.0F));
    checkClose(camera.direction(), glm::vec3(0.0F, 0.0F, -1.0F));
}

/**
 * The projection keeps the horizontal field of view it is given at any aspect. A point 45
 * degrees to the right reaches the right edge of clip space under a 90 degree field of view,
 * and the vertical scale is the horizontal one times the aspect.
 **/
BOOST_AUTO_TEST_CASE(camera_perspective_keeps_its_horizontal_field_of_view_test) {
    for (const float aspect : { 1.0F, 2.0F }) {
        BOOST_TEST_CONTEXT("aspect " << aspect) {
            Camera camera;
            camera.perspective({.fovx = 90.0F, .aspect = aspect, .znear = 0.1F, .zfar = 100.0F});
            const glm::vec4 clip = camera.projection() * glm::vec4(1.0F, 0.0F, -1.0F, 1.0F);
            BOOST_CHECK_CLOSE(clip.x / clip.w, 1.0F, 0.01F);
            // y is negated, because vulkan's clip space has y pointing down
            BOOST_CHECK_CLOSE(camera.projection()[1][1], -aspect, 0.01F);
        }
    }
}

};  // namespace

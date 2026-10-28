/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/Camera.h>
#include <api/render/offline/CameraOptions.h>
#include <api/render/offline/trace/Scene.h>

#include <map>
#include <string>

#include <boost/test/unit_test.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

/** A camera four units back from the world origin, with a screen that halves x. **/
v3d::render::offline::Camera placed() {
    v3d::render::offline::Camera camera;
    camera.view = glm::translate(glm::mat4x4(1.0F), glm::vec3(0.0F, 0.0F, 4.0F));
    camera.screen = glm::scale(glm::mat4x4(1.0F), glm::vec3(0.5F, 1.0F, 1.0F));
    return camera;
}

/**
 * A space the camera defines is answered from world space and from camera space. From world
 * space each goes through the view first. From camera space "camera" is exactly the identity
 * and "world" is the view's inverse.
 **/
BOOST_AUTO_TEST_CASE(camera_answers_its_spaces_from_world_and_camera_test) {
    const v3d::render::offline::Camera camera = placed();

    glm::mat4x4 matrix(0.0F);
    BOOST_REQUIRE(camera.fromWorld("screen", &matrix));
    BOOST_TEST((matrix == camera.screen * camera.view));
    BOOST_REQUIRE(camera.fromCamera("screen", &matrix));
    BOOST_TEST((matrix == camera.screen));
    BOOST_REQUIRE(camera.fromCamera("camera", &matrix));
    BOOST_TEST((matrix == glm::mat4x4(1.0F)));
    BOOST_REQUIRE(camera.fromCamera("world", &matrix));
    BOOST_TEST((matrix == glm::inverse(camera.view)));
    BOOST_CHECK_EQUAL(camera.eye().z, -4.0F);

    // a name the camera does not define leaves the matrix alone
    matrix = glm::mat4x4(2.0F);
    BOOST_CHECK(!camera.fromWorld("lamp", &matrix));
    BOOST_CHECK(!camera.fromCamera("lamp", &matrix));
    BOOST_TEST((matrix == glm::mat4x4(2.0F)));
}

/**
 * The traced scene answers a space the scene named as well as the camera's. A named space is
 * held as its transformation into world space, so the answer from world space is its inverse.
 **/
BOOST_AUTO_TEST_CASE(camera_a_scene_answers_a_named_space_test) {
    v3d::render::offline::trace::Scene scene;
    scene.camera(placed());
    const glm::mat4x4 lamp = glm::translate(glm::mat4x4(1.0F), glm::vec3(1.0F, 2.0F, 3.0F));
    scene.named(std::map<std::string, glm::mat4x4>{ { "lamp", lamp } });

    glm::mat4x4 matrix(0.0F);
    BOOST_REQUIRE(scene.fromWorld("lamp", &matrix));
    const glm::vec4 world(1.0F, 2.0F, 3.0F, 1.0F);
    BOOST_TEST((matrix * world == glm::vec4(0.0F, 0.0F, 0.0F, 1.0F)));
    BOOST_REQUIRE(scene.fromCamera("lamp", &matrix));
    const glm::vec4 seen = scene.camera().view * world;
    BOOST_TEST((matrix * seen == glm::vec4(0.0F, 0.0F, 0.0F, 1.0F)));
    BOOST_REQUIRE(scene.fromWorld("screen", &matrix));
    BOOST_CHECK(!scene.fromWorld("nowhere", &matrix));
}

/**
 * Freezing the options builds raster from screen: the screen window's upper left corner is
 * pixel (0, 0), and its lower right is the picture's size. The camera takes the options'
 * perspective, near plane and size.
 **/
BOOST_AUTO_TEST_CASE(camera_options_freeze_raster_from_the_screen_window_test) {
    v3d::render::offline::CameraOptions options;
    BOOST_REQUIRE(options.format({64, 48}, 1.0F));
    options.clipping({.hither = 0.5F, .yon = 10.0F});
    BOOST_REQUIRE(options.projection("orthographic", v3d::render::offline::CameraOptions::defaultFov, glm::mat4x4(1.0F)));
    const v3d::render::offline::Camera camera = options.freeze(glm::mat4x4(1.0F));

    const glm::vec4 upperLeft = camera.raster * glm::vec4(-4.0F / 3.0F, 1.0F, 1.0F, 1.0F);
    const glm::vec4 lowerRight = camera.raster * glm::vec4(4.0F / 3.0F, -1.0F, 1.0F, 1.0F);
    BOOST_CHECK_SMALL(upperLeft.x, 1.0e-4F);
    BOOST_CHECK_SMALL(upperLeft.y, 1.0e-4F);
    BOOST_CHECK_CLOSE(lowerRight.x, 64.0F, 0.001F);
    BOOST_CHECK_CLOSE(lowerRight.y, 48.0F, 0.001F);
    BOOST_CHECK(!camera.perspective);
    BOOST_CHECK_EQUAL(camera.hither, 0.5F);
    BOOST_CHECK_EQUAL(camera.width, 64U);
    BOOST_CHECK_EQUAL(camera.height, 48U);

    // an unknown projection is refused, and the one in force stays
    BOOST_CHECK(!options.projection("fisheye", 90.0F, glm::mat4x4(1.0F)));
    BOOST_CHECK(!options.perspective());
}

};  // namespace

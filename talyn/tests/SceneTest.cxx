/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../libtalyn/Scene.h"

BOOST_AUTO_TEST_CASE(scene_test) {
    v3d::talyn::Scene scene;

    // a ray that hits nothing is worth the background, which starts black
    BOOST_CHECK_EQUAL(scene.background().r, 0.0f);
    BOOST_CHECK_EQUAL(scene.triangles().size(), 0u);

    scene.background(glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(scene.background().b, 1.0f);

    v3d::talyn::Triangle triangle(
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(1.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 1.0f, 1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f));
    scene.add(triangle);

    BOOST_REQUIRE_EQUAL(scene.triangles().size(), 1u);
    BOOST_CHECK_EQUAL(scene.triangles()[0].b().x, 1.0f);
    BOOST_CHECK_EQUAL(scene.triangles()[0].c().y, 1.0f);
    BOOST_CHECK_EQUAL(scene.triangles()[0].colour().r, 1.0f);

    // the camera is the scene's own, and reads back what was written to its profile
    scene.camera().profile().eye(glm::vec3(0.0f, 0.0f, -4.0f));
    BOOST_CHECK_EQUAL(scene.camera().profile().eye().z, -4.0f);
}

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>

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

/**
 * A triangle that was told nothing about its normals lies flat: its plane is both Ng and the N
 * a hit anywhere on it shades with.
 **/
BOOST_AUTO_TEST_CASE(triangle_geometric_normal_test) {
    v3d::talyn::Triangle triangle(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(1.0f));

    BOOST_TEST((triangle.geometricNormal() == glm::vec3(0.0f, 0.0f, 1.0f)));
    BOOST_TEST((triangle.shadingNormal(0.0f, 0.0f) == glm::vec3(0.0f, 0.0f, 1.0f)));
    BOOST_TEST((triangle.shadingNormal(0.3f, 0.3f) == glm::vec3(0.0f, 0.0f, 1.0f)));

    // the winding decides which way it faces
    v3d::talyn::Triangle reversed(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(1.0f));

    BOOST_TEST((reversed.geometricNormal() == glm::vec3(0.0f, 0.0f, -1.0f)));

    // a triangle with no area lies in no plane and answers zero rather than a normalised
    // nothing
    v3d::talyn::Triangle degenerate(
        glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(1.0f));

    BOOST_TEST((degenerate.geometricNormal() == glm::vec3(0.0f)));
}

/**
 * A triangle given a normal per corner interpolates between them, which is what makes a surface
 * smooth. Ng stays its plane: SL defines faceforward() and calculatenormal() in terms of both.
 **/
BOOST_AUTO_TEST_CASE(triangle_shading_normal_test) {
    const glm::vec3 leaning = glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f));
    v3d::talyn::Triangle triangle(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(1.0f),
        leaning,
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, 1.0f));

    BOOST_TEST((triangle.geometricNormal() == glm::vec3(0.0f, 0.0f, 1.0f)));

    // u weighs b and v weighs c, so the origin of the pair is the corner a
    const glm::vec3 atA = triangle.shadingNormal(0.0f, 0.0f);
    BOOST_TEST(atA.x == leaning.x, boost::test_tools::tolerance(0.0001f));

    const glm::vec3 atB = triangle.shadingNormal(1.0f, 0.0f);
    BOOST_TEST(atB.x == 0.0f, boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(atB.z == 1.0f, boost::test_tools::tolerance(0.0001f));

    // between them it is neither, and interpolating unit normals does not give a unit one
    // back on its own
    const glm::vec3 middle = triangle.shadingNormal(0.5f, 0.0f);
    BOOST_TEST(middle.x > 0.0f);
    BOOST_TEST(middle.x < leaning.x);
    BOOST_TEST(glm::length(middle) == 1.0f, boost::test_tools::tolerance(0.0001f));
}

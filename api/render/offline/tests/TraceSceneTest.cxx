/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/MovingTransform.h>
#include <api/render/offline/trace/Scene.h>

#include <cmath>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

BOOST_AUTO_TEST_CASE(scene_test) {
    v3d::render::offline::trace::Scene scene;

    // a ray that hits nothing is worth the background, which starts black
    BOOST_CHECK_EQUAL(scene.background().r, 0.0f);
    BOOST_CHECK_EQUAL(scene.triangles().size(), 0u);

    scene.background(glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(scene.background().b, 1.0f);

    v3d::render::offline::trace::Triangle triangle(
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
    v3d::render::offline::trace::Triangle triangle(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(1.0f));

    BOOST_TEST((triangle.geometricNormal() == glm::vec3(0.0f, 0.0f, 1.0f)));
    BOOST_TEST((triangle.shadingNormal(0.0f, 0.0f) == glm::vec3(0.0f, 0.0f, 1.0f)));
    BOOST_TEST((triangle.shadingNormal(0.3f, 0.3f) == glm::vec3(0.0f, 0.0f, 1.0f)));

    // the winding decides which way it faces
    v3d::render::offline::trace::Triangle reversed(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(1.0f));

    BOOST_TEST((reversed.geometricNormal() == glm::vec3(0.0f, 0.0f, -1.0f)));

    // a triangle with no area lies in no plane and answers zero rather than a normalised
    // nothing
    v3d::render::offline::trace::Triangle degenerate(
        glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(1.0f));

    BOOST_TEST((degenerate.geometricNormal() == glm::vec3(0.0f)));
}

/**
 * A triangle given a normal per corner interpolates between them, which is what makes a surface
 * smooth. Ng stays its plane: SL defines faceforward() and calculatenormal() in terms of both.
 **/
BOOST_AUTO_TEST_CASE(triangle_shading_normal_test) {
    const glm::vec3 leaning = glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f));
    v3d::render::offline::trace::Triangle triangle(
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

namespace {

/** Whether a ray straight down the negative z axis from z = 10 meets the scene, and where. **/
bool downward(const v3d::render::offline::trace::Scene & scene, float x, float y, v3d::render::offline::trace::Hit* hit) {
    return scene.nearest(v3d::type::geometry::Ray(glm::vec3(x, y, 10.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        0.0f, hit);
}

};  // namespace

/**
 * A sphere's silhouette is its radius, exactly, wherever it was placed: a ray just inside
 * the edge meets it and one just outside does not. Its normal points out, and it is met on
 * the near side first.
 **/
BOOST_AUTO_TEST_CASE(scene_sphere_silhouette_test) {
    v3d::render::offline::trace::Scene scene;
    const glm::mat4x4 placed = glm::translate(glm::mat4x4(1.0f), glm::vec3(2.0f, 1.0f, 0.0f));
    scene.add(v3d::render::offline::trace::Sphere(1.5f, -1.5f, 1.5f, 360.0f, placed, glm::vec3(1.0f)),
        v3d::render::offline::MovingTransform(placed));
    BOOST_REQUIRE_EQUAL(scene.spheres().size(), 1u);

    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE(downward(scene, 2.0f, 1.0f, &hit));
    BOOST_CHECK_CLOSE(hit.distance, 8.5f, 0.001f);
    BOOST_CHECK_CLOSE(hit.normal.z, 1.0f, 0.001f);
    BOOST_CHECK(hit.primitive == scene.spheres().data());

    BOOST_CHECK(downward(scene, 2.0f + 1.49f, 1.0f, &hit));
    BOOST_CHECK(!downward(scene, 2.0f + 1.51f, 1.0f, &hit));
    BOOST_CHECK(downward(scene, 2.0f, 1.0f - 1.49f, &hit));
    BOOST_CHECK(!downward(scene, 2.0f, 1.0f - 1.51f, &hit));
}

/**
 * RI's cut sphere: a slab of heights and a sweep about z. A ray down through a sphere cut
 * off at half its height goes in through the open top and meets the inside of the bottom;
 * one where the sweep has not reached meets nothing at all.
 **/
BOOST_AUTO_TEST_CASE(scene_sphere_cut_test) {
    v3d::render::offline::trace::Scene scene;
    scene.add(v3d::render::offline::trace::Sphere(1.0f, -1.0f, 0.5f, 180.0f, glm::mat4x4(1.0f), glm::vec3(1.0f)),
        v3d::render::offline::MovingTransform());

    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE(downward(scene, 0.0f, 0.2f, &hit));
    BOOST_CHECK_CLOSE(hit.point.z, -std::sqrt(1.0f - 0.04f), 0.01f);
    // the outward normal of the bottom, which is the inside the ray is looking at
    BOOST_CHECK_LT(hit.normal.z, 0.0f);
    // a negative y is past a sweep of half a turn from the x axis
    BOOST_CHECK(!downward(scene, 0.0f, -0.2f, &hit));
}

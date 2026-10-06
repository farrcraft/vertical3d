/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/MovingTransform.h>
#include <api/render/offline/trace/Hit.h>
#include <api/render/offline/trace/Intersection.h>
#include <api/render/offline/trace/Pose.h>
#include <api/render/offline/trace/Scene.h>
#include <api/render/offline/trace/Sphere.h>
#include <api/render/offline/trace/Triangle.h>

#include <cmath>

#include <boost/make_shared.hpp>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {

/**
 * The plane z = 0, a primitive defined only in this test. A primitive is anything that
 * intersects a ray and describes the hit, and a scene accepts any of them.
 **/
class Floor final : public v3d::render::offline::trace::Primitive {
 public:
    Floor() : Primitive(glm::vec3(0.5f)) {
    }

    bool intersect(const v3d::type::geometry::Ray & ray, float from, const v3d::render::offline::trace::Pose & /* pose */,
        v3d::render::offline::trace::Intersection* found) const override {
        if (ray.direction().z == 0.0f) {
            return false;
        }
        const float along = -ray.origin().z / ray.direction().z;
        if (along <= from) {
            return false;
        }
        found->distance = along;
        return true;
    }

    void describe(const v3d::render::offline::trace::Intersection & /* found */, v3d::render::offline::trace::Hit* hit) const override {
        hit->normal = glm::vec3(0.0f, 0.0f, 1.0f);
        hit->geometric = hit->normal;
    }
};

};  // namespace

BOOST_AUTO_TEST_CASE(scene_test) {
    v3d::render::offline::trace::Scene scene;

    // a ray that hits nothing returns the background, which starts black
    BOOST_CHECK_EQUAL(scene.background().r, 0.0f);
    BOOST_CHECK_EQUAL(scene.all<v3d::render::offline::trace::Triangle>().size(), 0u);

    scene.background(glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(scene.background().b, 1.0f);

    v3d::render::offline::trace::Triangle triangle(
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(1.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 1.0f, 1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f));
    scene.add(triangle);

    BOOST_REQUIRE_EQUAL(scene.all<v3d::render::offline::trace::Triangle>().size(), 1u);
    BOOST_CHECK_EQUAL(scene.all<v3d::render::offline::trace::Triangle>()[0]->b().x, 1.0f);
    BOOST_CHECK_EQUAL(scene.all<v3d::render::offline::trace::Triangle>()[0]->c().y, 1.0f);
    BOOST_CHECK_EQUAL(scene.all<v3d::render::offline::trace::Triangle>()[0]->colour().r, 1.0f);

    // the eye is where the world to camera transformation puts the camera space origin back
    scene.view(glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, 0.0f, 4.0f)));
    BOOST_CHECK_EQUAL(scene.eye().z, -4.0f);
}

/**
 * A triangle given no normals is flat: its plane is both Ng and the N a hit anywhere on it
 * shades with.
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

    // a triangle with no area lies in no plane and returns zero rather than normalising a
    // zero vector
    v3d::render::offline::trace::Triangle degenerate(
        glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(1.0f));

    BOOST_TEST((degenerate.geometricNormal() == glm::vec3(0.0f)));
}

/**
 * A triangle given a normal per corner interpolates between them, so the surface shades
 * smoothly. Ng stays its plane: SL defines faceforward() and calculatenormal() in terms of both.
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
    BOOST_REQUIRE_EQUAL(scene.all<v3d::render::offline::trace::Sphere>().size(), 1u);

    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE(downward(scene, 2.0f, 1.0f, &hit));
    BOOST_CHECK_CLOSE(hit.distance, 8.5f, 0.001f);
    BOOST_CHECK_CLOSE(hit.normal.z, 1.0f, 0.001f);
    BOOST_CHECK(hit.primitive == scene.all<v3d::render::offline::trace::Sphere>()[0]);

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
    // the outward normal of the bottom, whose inside the ray meets
    BOOST_CHECK_LT(hit.normal.z, 0.0f);
    // a negative y is past a sweep of half a turn from the x axis
    BOOST_CHECK(!downward(scene, 0.0f, -0.2f, &hit));
}

/**
 * A triangle given a colour at each corner is hit with those colours weighted as its normals
 * are, and one given none is hit with the colour it was built with.
 **/
BOOST_AUTO_TEST_CASE(triangle_corner_colours_test) {
    v3d::render::offline::trace::Triangle graded(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f));
    graded.colours(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    v3d::render::offline::trace::Scene scene;
    scene.add(graded);

    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE(downward(scene, 0.25f, 0.5f, &hit));
    BOOST_CHECK_CLOSE(hit.colour.r, 0.25f, 0.01f);
    BOOST_CHECK_CLOSE(hit.colour.g, 0.25f, 0.01f);
    BOOST_CHECK_CLOSE(hit.colour.b, 0.5f, 0.01f);

    v3d::render::offline::trace::Scene flat;
    flat.add(v3d::render::offline::trace::Triangle(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.5f, 0.25f, 0.125f)));
    BOOST_REQUIRE(downward(flat, 0.25f, 0.5f, &hit));
    BOOST_CHECK_CLOSE(hit.colour.g, 0.25f, 0.01f);
}

/**
 * A sphere whose radius is negative or not a number can be built and is never met.
 **/
BOOST_AUTO_TEST_CASE(scene_sphere_with_no_size_test) {
    v3d::render::offline::trace::Scene scene;
    scene.add(v3d::render::offline::trace::Sphere(-1.0f, -1.0f, 1.0f, 360.0f, glm::mat4x4(1.0f), glm::vec3(1.0f)),
        v3d::render::offline::MovingTransform());
    scene.add(v3d::render::offline::trace::Sphere(std::nanf(""), -1.0f, 1.0f, 360.0f, glm::mat4x4(1.0f),
        glm::vec3(1.0f)), v3d::render::offline::MovingTransform());

    v3d::render::offline::trace::Hit hit;
    BOOST_CHECK(!downward(scene, 0.0f, 0.0f, &hit));
}

/**
 * A kind of primitive defined outside the library is met, nearest first among the rest, and
 * describes its own hit.
 **/
BOOST_AUTO_TEST_CASE(scene_any_primitive_test) {
    v3d::render::offline::trace::Scene scene;
    const boost::shared_ptr<Floor> floor = boost::make_shared<Floor>();
    scene.add(floor, v3d::render::offline::MovingTransform());
    scene.add(v3d::render::offline::trace::Triangle(glm::vec3(-1.0f, -1.0f, -2.0f), glm::vec3(1.0f, -1.0f, -2.0f),
        glm::vec3(0.0f, 1.0f, -2.0f), glm::vec3(1.0f)));

    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE(scene.nearest(v3d::type::geometry::Ray(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f)), 0.0f, &hit));
    BOOST_CHECK(hit.primitive == floor.get());
    BOOST_CHECK_CLOSE(hit.distance, 5.0f, 0.001f);
    BOOST_CHECK_EQUAL(hit.normal.z, 1.0f);
    BOOST_CHECK_EQUAL(scene.all<Floor>().size(), 1u);
    BOOST_CHECK_EQUAL(scene.all<v3d::render::offline::trace::Triangle>().size(), 1u);

    // past it, the triangle behind
    BOOST_REQUIRE(scene.nearest(v3d::type::geometry::Ray(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f)), 5.0f, &hit));
    BOOST_CHECK(hit.primitive == scene.all<v3d::render::offline::trace::Triangle>()[0]);
}

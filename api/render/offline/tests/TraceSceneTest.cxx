/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/MovingTransform.h>
#include <api/render/offline/trace/Hit.h>
#include <api/render/offline/trace/Intersection.h>
#include <api/render/offline/trace/Pose.h>
#include <api/render/offline/trace/Primitive.h>
#include <api/render/offline/trace/Scene.h>
#include <api/render/offline/trace/Sphere.h>
#include <api/render/offline/trace/Triangle.h>
#include <api/type/geometry/Ray.h>

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
    Floor() : Primitive(glm::vec3(0.5F)) {
    }

    bool intersect(const v3d::type::geometry::Ray & ray, float from, const v3d::render::offline::trace::Pose & /* pose */,
        v3d::render::offline::trace::Intersection* found) const override {
        if (ray.direction().z == 0.0F) {
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
        hit->normal = glm::vec3(0.0F, 0.0F, 1.0F);
        hit->geometric = hit->normal;
    }
};

BOOST_AUTO_TEST_CASE(scene_test) {
    v3d::render::offline::trace::Scene scene;

    // a ray that hits nothing returns the background, which starts black
    BOOST_CHECK_EQUAL(scene.background().r, 0.0F);
    BOOST_CHECK_EQUAL(scene.all<v3d::render::offline::trace::Triangle>().size(), 0U);

    scene.background(glm::vec3(0.0F, 0.0F, 1.0F));
    BOOST_CHECK_EQUAL(scene.background().b, 1.0F);

    const v3d::render::offline::trace::Triangle triangle({glm::vec3(0.0F, 0.0F, 1.0F), glm::vec3(1.0F, 0.0F, 1.0F), glm::vec3(0.0F, 1.0F, 1.0F)}, glm::vec3(1.0F, 0.0F, 0.0F));
    scene.add(triangle);

    BOOST_REQUIRE_EQUAL(scene.all<v3d::render::offline::trace::Triangle>().size(), 1U);
    BOOST_CHECK_EQUAL(scene.all<v3d::render::offline::trace::Triangle>()[0]->b().x, 1.0F);
    BOOST_CHECK_EQUAL(scene.all<v3d::render::offline::trace::Triangle>()[0]->c().y, 1.0F);
    BOOST_CHECK_EQUAL(scene.all<v3d::render::offline::trace::Triangle>()[0]->colour().r, 1.0F);

    // the eye is where the world to camera transformation puts the camera space origin back
    scene.view(glm::translate(glm::mat4x4(1.0F), glm::vec3(0.0F, 0.0F, 4.0F)));
    BOOST_CHECK_EQUAL(scene.eye().z, -4.0F);
}

/**
 * A triangle given no normals is flat: its plane is both Ng and the N a hit anywhere on it
 * shades with.
 **/
BOOST_AUTO_TEST_CASE(triangle_geometric_normal_test) {
    const v3d::render::offline::trace::Triangle triangle({glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(1.0F, 0.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F)}, glm::vec3(1.0F));

    BOOST_TEST((triangle.geometricNormal() == glm::vec3(0.0F, 0.0F, 1.0F)));
    BOOST_TEST((triangle.shadingNormal(0.0F, 0.0F) == glm::vec3(0.0F, 0.0F, 1.0F)));
    BOOST_TEST((triangle.shadingNormal(0.3F, 0.3F) == glm::vec3(0.0F, 0.0F, 1.0F)));

    // the winding decides which way it faces
    const v3d::render::offline::trace::Triangle reversed({glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(1.0F, 0.0F, 0.0F)}, glm::vec3(1.0F));

    BOOST_TEST((reversed.geometricNormal() == glm::vec3(0.0F, 0.0F, -1.0F)));

    // a triangle with no area lies in no plane and returns zero rather than normalising a
    // zero vector
    const v3d::render::offline::trace::Triangle degenerate({glm::vec3(0.0F), glm::vec3(1.0F, 0.0F, 0.0F), glm::vec3(2.0F, 0.0F, 0.0F)}, glm::vec3(1.0F));

    BOOST_TEST((degenerate.geometricNormal() == glm::vec3(0.0F)));
}

/**
 * A triangle given a normal per corner interpolates between them, so the surface shades
 * smoothly. Ng stays its plane: SL defines faceforward() and calculatenormal() in terms of both.
 **/
BOOST_AUTO_TEST_CASE(triangle_shading_normal_test) {
    const glm::vec3 leaning = glm::normalize(glm::vec3(1.0F, 0.0F, 1.0F));
    const v3d::render::offline::trace::Triangle triangle({glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(1.0F, 0.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F)},
        glm::vec3(1.0F), {leaning, glm::vec3(0.0F, 0.0F, 1.0F), glm::vec3(0.0F, 0.0F, 1.0F)});

    BOOST_TEST((triangle.geometricNormal() == glm::vec3(0.0F, 0.0F, 1.0F)));

    // u weighs b and v weighs c, so the origin of the pair is the corner a
    const glm::vec3 atA = triangle.shadingNormal(0.0F, 0.0F);
    BOOST_TEST(atA.x == leaning.x, boost::test_tools::tolerance(0.0001F));

    const glm::vec3 atB = triangle.shadingNormal(1.0F, 0.0F);
    BOOST_TEST(atB.x == 0.0F, boost::test_tools::tolerance(0.0001F));
    BOOST_TEST(atB.z == 1.0F, boost::test_tools::tolerance(0.0001F));

    // between them it is neither, and interpolating unit normals does not give a unit one
    // back on its own
    const glm::vec3 middle = triangle.shadingNormal(0.5F, 0.0F);
    BOOST_TEST(middle.x > 0.0F);
    BOOST_TEST(middle.x < leaning.x);
    BOOST_TEST(glm::length(middle) == 1.0F, boost::test_tools::tolerance(0.0001F));
}

namespace {

/** Whether a ray straight down the negative z axis from z = 10 meets the scene, and where. **/
bool downward(const v3d::render::offline::trace::Scene & scene, float x, float y, v3d::render::offline::trace::Hit* hit) {
    return scene.nearest(v3d::type::geometry::Ray({.origin = glm::vec3(x, y, 10.0F), .direction = glm::vec3(0.0F, 0.0F, -1.0F)}),
        0.0F, hit);
}

};  // namespace

/**
 * A sphere's silhouette is its radius, exactly, wherever it was placed: a ray just inside
 * the edge meets it and one just outside does not. Its normal points out, and it is met on
 * the near side first.
 **/
BOOST_AUTO_TEST_CASE(scene_sphere_silhouette_test) {
    v3d::render::offline::trace::Scene scene;
    const glm::mat4x4 placed = glm::translate(glm::mat4x4(1.0F), glm::vec3(2.0F, 1.0F, 0.0F));
    scene.add(v3d::render::offline::trace::Sphere({.radius = 1.5F, .zmin = -1.5F, .zmax = 1.5F, .thetamax = 360.0F}, placed, glm::vec3(1.0F)),
        v3d::render::offline::MovingTransform(placed));
    BOOST_REQUIRE_EQUAL(scene.all<v3d::render::offline::trace::Sphere>().size(), 1U);

    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE(downward(scene, 2.0F, 1.0F, &hit));
    BOOST_CHECK_CLOSE(hit.distance, 8.5F, 0.001F);
    BOOST_CHECK_CLOSE(hit.normal.z, 1.0F, 0.001F);
    BOOST_CHECK(hit.primitive == scene.all<v3d::render::offline::trace::Sphere>()[0]);

    BOOST_CHECK(downward(scene, 2.0F + 1.49F, 1.0F, &hit));
    BOOST_CHECK(!downward(scene, 2.0F + 1.51F, 1.0F, &hit));
    BOOST_CHECK(downward(scene, 2.0F, 1.0F - 1.49F, &hit));
    BOOST_CHECK(!downward(scene, 2.0F, 1.0F - 1.51F, &hit));
}

/**
 * RI's cut sphere: a slab of heights and a sweep about z. A ray down through a sphere cut
 * off at half its height goes in through the open top and meets the inside of the bottom;
 * one where the sweep has not reached meets nothing at all.
 **/
BOOST_AUTO_TEST_CASE(scene_sphere_cut_test) {
    v3d::render::offline::trace::Scene scene;
    scene.add(v3d::render::offline::trace::Sphere({.radius = 1.0F, .zmin = -1.0F, .zmax = 0.5F, .thetamax = 180.0F}, glm::mat4x4(1.0F), glm::vec3(1.0F)),
        v3d::render::offline::MovingTransform());

    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE(downward(scene, 0.0F, 0.2F, &hit));
    BOOST_CHECK_CLOSE(hit.point.z, -std::sqrt(1.0F - 0.04F), 0.01F);
    // the outward normal of the bottom, whose inside the ray meets
    BOOST_CHECK_LT(hit.normal.z, 0.0F);
    // a negative y is past a sweep of half a turn from the x axis
    BOOST_CHECK(!downward(scene, 0.0F, -0.2F, &hit));
}

/**
 * A triangle given a colour at each corner is hit with those colours weighted as its normals
 * are, and one given none is hit with the colour it was built with.
 **/
BOOST_AUTO_TEST_CASE(triangle_corner_colours_test) {
    v3d::render::offline::trace::Triangle graded({glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(1.0F, 0.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F)}, glm::vec3(1.0F));
    graded.colours(glm::vec3(1.0F, 0.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(0.0F, 0.0F, 1.0F));
    v3d::render::offline::trace::Scene scene;
    scene.add(graded);

    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE(downward(scene, 0.25F, 0.5F, &hit));
    BOOST_CHECK_CLOSE(hit.colour.r, 0.25F, 0.01F);
    BOOST_CHECK_CLOSE(hit.colour.g, 0.25F, 0.01F);
    BOOST_CHECK_CLOSE(hit.colour.b, 0.5F, 0.01F);

    v3d::render::offline::trace::Scene flat;
    flat.add(v3d::render::offline::trace::Triangle({glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(1.0F, 0.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F)}, glm::vec3(0.5F, 0.25F, 0.125F)));
    BOOST_REQUIRE(downward(flat, 0.25F, 0.5F, &hit));
    BOOST_CHECK_CLOSE(hit.colour.g, 0.25F, 0.01F);
}

/**
 * A sphere whose radius is negative or not a number can be built and is never met.
 **/
BOOST_AUTO_TEST_CASE(scene_sphere_with_no_size_test) {
    v3d::render::offline::trace::Scene scene;
    scene.add(v3d::render::offline::trace::Sphere({.radius = -1.0F, .zmin = -1.0F, .zmax = 1.0F, .thetamax = 360.0F}, glm::mat4x4(1.0F), glm::vec3(1.0F)),
        v3d::render::offline::MovingTransform());
    scene.add(v3d::render::offline::trace::Sphere({.radius = std::nanf(""), .zmin = -1.0F, .zmax = 1.0F, .thetamax = 360.0F}, glm::mat4x4(1.0F), glm::vec3(1.0F)),
        v3d::render::offline::MovingTransform());

    v3d::render::offline::trace::Hit hit;
    BOOST_CHECK(!downward(scene, 0.0F, 0.0F, &hit));
}

/**
 * A kind of primitive defined outside the library is met, nearest first among the rest, and
 * describes its own hit.
 **/
BOOST_AUTO_TEST_CASE(scene_any_primitive_test) {
    v3d::render::offline::trace::Scene scene;
    const boost::shared_ptr<Floor> floor = boost::make_shared<Floor>();
    scene.add(floor, v3d::render::offline::MovingTransform());
    scene.add(v3d::render::offline::trace::Triangle({glm::vec3(-1.0F, -1.0F, -2.0F), glm::vec3(1.0F, -1.0F, -2.0F), glm::vec3(0.0F, 1.0F, -2.0F)}, glm::vec3(1.0F)));

    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE(scene.nearest(v3d::type::geometry::Ray({.origin = glm::vec3(0.0F, 0.0F, 5.0F), .direction = glm::vec3(0.0F, 0.0F, -1.0F)}), 0.0F, &hit));
    BOOST_CHECK(hit.primitive == floor.get());
    BOOST_CHECK_CLOSE(hit.distance, 5.0F, 0.001F);
    BOOST_CHECK_EQUAL(hit.normal.z, 1.0F);
    BOOST_CHECK_EQUAL(scene.all<Floor>().size(), 1U);
    BOOST_CHECK_EQUAL(scene.all<v3d::render::offline::trace::Triangle>().size(), 1U);

    // past it, the triangle behind
    BOOST_REQUIRE(scene.nearest(v3d::type::geometry::Ray({.origin = glm::vec3(0.0F, 0.0F, 5.0F), .direction = glm::vec3(0.0F, 0.0F, -1.0F)}), 5.0F, &hit));
    BOOST_CHECK(hit.primitive == scene.all<v3d::render::offline::trace::Triangle>()[0]);
}

};  // namespace

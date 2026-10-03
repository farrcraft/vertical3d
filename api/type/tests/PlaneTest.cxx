/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/geometry/AABBox.h>
#include <api/type/geometry/Plane.h>

#include <boost/test/unit_test.hpp>

#include <glm/glm.hpp>

namespace {

/**
 * The z = 0 plane, keeping the positive half space.
 **/
v3d::type::geometry::Plane zPlane() {
    v3d::type::geometry::Plane plane;
    plane.calculate(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    return plane;
}

};  // namespace

/**
 * The equation is the plane's only state, so a plane built from a normal and a point reports
 * back the normal and distance it was given rather than a second, separately written copy.
 **/
BOOST_AUTO_TEST_CASE(plane_representation_test) {
    v3d::type::geometry::Plane plane;
    plane.calculate(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 5.0f, 0.0f));

    BOOST_TEST((plane.normal() == glm::vec3(0.0f, 1.0f, 0.0f)));
    BOOST_TEST(plane.distance() == 5.0f);
    BOOST_TEST(plane.distance(glm::vec3(0.0f, 7.0f, 0.0f)) == 2.0f);
}

/**
 * The three point constructor writes the same equation the two argument calculate does, so
 * classify reads a plane that was filled in.
 **/
BOOST_AUTO_TEST_CASE(plane_from_three_points_test) {
    v3d::type::geometry::Plane plane(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(1.0f, 0.0f, 2.0f), glm::vec3(1.0f, 1.0f, 2.0f));

    BOOST_TEST(plane.classify(glm::vec3(0.0f, 0.0f, 5.0f)) == v3d::type::geometry::Plane::POSITIVE);
    BOOST_TEST(plane.classify(glm::vec3(0.0f, 0.0f, 0.0f)) == v3d::type::geometry::Plane::NEGATIVE);
    BOOST_TEST(plane.classify(glm::vec3(4.0f, 4.0f, 2.0f)) == v3d::type::geometry::Plane::ON_PLANE);
}

/**
 * A ray hit is found from the same equation the classification is, so the two agree about
 * where the plane is.
 **/
BOOST_AUTO_TEST_CASE(plane_intersect_edge_test) {
    v3d::type::geometry::Plane plane = zPlane();

    glm::vec3 hit;
    BOOST_TEST(plane.intersectEdge(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f), &hit));
    BOOST_TEST((hit == glm::vec3(0.0f, 0.0f, 0.0f)));

    // an edge that stays on one side is not crossed
    BOOST_TEST(!plane.intersectEdge(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 2.0f), &hit));
}

/**
 * A box is inside only if all eight of its corners are, outside only if none are, and
 * crossing otherwise. A frustum culls with this, so a box straddling one plane must not be
 * reported as outside it.
 **/
BOOST_AUTO_TEST_CASE(plane_classify_box_test) {
    const v3d::type::geometry::Plane plane = zPlane();

    v3d::type::geometry::AABBox above;
    above.min(glm::vec3(-1.0f, -1.0f, 1.0f));
    above.max(glm::vec3(1.0f, 1.0f, 2.0f));
    BOOST_TEST(plane.classify(above) == v3d::type::geometry::Plane::INSIDE);

    v3d::type::geometry::AABBox below;
    below.min(glm::vec3(-1.0f, -1.0f, -2.0f));
    below.max(glm::vec3(1.0f, 1.0f, -1.0f));
    BOOST_TEST(plane.classify(below) == v3d::type::geometry::Plane::OUTSIDE);

    v3d::type::geometry::AABBox straddling;
    straddling.min(glm::vec3(-1.0f, -1.0f, -1.0f));
    straddling.max(glm::vec3(1.0f, 1.0f, 1.0f));
    BOOST_TEST(plane.classify(straddling) == v3d::type::geometry::Plane::CROSSING);
}

/**
 * The const subscript reads the same equation the writing one fills in.
 **/
BOOST_AUTO_TEST_CASE(plane_const_subscript_test) {
    v3d::type::geometry::Plane plane;
    plane[0] = 0.0f;
    plane[1] = 2.0f;
    plane[2] = 0.0f;
    plane[3] = -4.0f;

    const v3d::type::geometry::Plane& reader = plane;
    BOOST_TEST(reader[v3d::type::geometry::Plane::B] == 2.0f);
    BOOST_TEST(reader[v3d::type::geometry::Plane::D] == -4.0f);
    BOOST_TEST(reader.distance(glm::vec3(0.0f, 3.0f, 0.0f)) == 2.0f);
}

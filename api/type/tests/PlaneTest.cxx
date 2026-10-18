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
    plane.calculate(glm::vec3(0.0F, 0.0F, 1.0F), glm::vec3(0.0F, 0.0F, 0.0F));
    return plane;
}

/**
 * The equation is the plane's only state, so a plane built from a normal and a point reports
 * back the normal and distance it was given rather than a second, separately written copy.
 **/
BOOST_AUTO_TEST_CASE(plane_representation_test) {
    v3d::type::geometry::Plane plane;
    plane.calculate(glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(0.0F, 5.0F, 0.0F));

    BOOST_TEST((plane.normal() == glm::vec3(0.0F, 1.0F, 0.0F)));
    BOOST_TEST(plane.distance() == 5.0F);
    BOOST_TEST(plane.signedDistance(glm::vec3(0.0F, 7.0F, 0.0F)) == 2.0F);
}

/**
 * The three point constructor writes the same equation the two argument calculate does, so
 * classify reads a plane that was filled in.
 **/
BOOST_AUTO_TEST_CASE(plane_from_three_points_test) {
    const v3d::type::geometry::Plane plane(glm::vec3(0.0F, 0.0F, 2.0F), glm::vec3(1.0F, 0.0F, 2.0F), glm::vec3(1.0F, 1.0F, 2.0F));

    BOOST_TEST((plane.classify(glm::vec3(0.0F, 0.0F, 5.0F)) == v3d::type::geometry::Plane::POSITIVE));
    BOOST_TEST((plane.classify(glm::vec3(0.0F, 0.0F, 0.0F)) == v3d::type::geometry::Plane::NEGATIVE));
    BOOST_TEST((plane.classify(glm::vec3(4.0F, 4.0F, 2.0F)) == v3d::type::geometry::Plane::ON_PLANE));
}

/**
 * A ray hit is found from the same equation the classification is, so the two agree about
 * where the plane is.
 **/
BOOST_AUTO_TEST_CASE(plane_intersect_edge_test) {
    const v3d::type::geometry::Plane plane = zPlane();

    glm::vec3 hit;
    BOOST_TEST(plane.intersectEdge(glm::vec3(0.0F, 0.0F, -1.0F), glm::vec3(0.0F, 0.0F, 1.0F), &hit));
    BOOST_TEST((hit == glm::vec3(0.0F, 0.0F, 0.0F)));

    // an edge that stays on one side is not crossed
    BOOST_TEST(!plane.intersectEdge(glm::vec3(0.0F, 0.0F, 1.0F), glm::vec3(0.0F, 0.0F, 2.0F), &hit));
}

/**
 * A box is inside only if all eight of its corners are, outside only if none are, and
 * crossing otherwise. A frustum culls with this, so a box straddling one plane must not be
 * reported as outside it.
 **/
BOOST_AUTO_TEST_CASE(plane_classify_box_test) {
    const v3d::type::geometry::Plane plane = zPlane();

    v3d::type::geometry::AABBox above;
    above.min(glm::vec3(-1.0F, -1.0F, 1.0F));
    above.max(glm::vec3(1.0F, 1.0F, 2.0F));
    BOOST_TEST((plane.classify(above) == v3d::type::geometry::Plane::INSIDE));

    v3d::type::geometry::AABBox below;
    below.min(glm::vec3(-1.0F, -1.0F, -2.0F));
    below.max(glm::vec3(1.0F, 1.0F, -1.0F));
    BOOST_TEST((plane.classify(below) == v3d::type::geometry::Plane::OUTSIDE));

    v3d::type::geometry::AABBox straddling;
    straddling.min(glm::vec3(-1.0F, -1.0F, -1.0F));
    straddling.max(glm::vec3(1.0F, 1.0F, 1.0F));
    BOOST_TEST((plane.classify(straddling) == v3d::type::geometry::Plane::CROSSING));
}

/**
 * The const subscript reads the same equation the writing one fills in.
 **/
BOOST_AUTO_TEST_CASE(plane_const_subscript_test) {
    v3d::type::geometry::Plane plane;
    plane[0] = 0.0F;
    plane[1] = 2.0F;
    plane[2] = 0.0F;
    plane[3] = -4.0F;

    const v3d::type::geometry::Plane& reader = plane;
    BOOST_TEST(reader[v3d::type::geometry::Plane::B] == 2.0F);
    BOOST_TEST(reader[v3d::type::geometry::Plane::D] == -4.0F);
    BOOST_TEST(reader.signedDistance(glm::vec3(0.0F, 3.0F, 0.0F)) == 2.0F);
}

};  // namespace

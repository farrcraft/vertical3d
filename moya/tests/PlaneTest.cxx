/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <moya/libmoya/Plane.h>
#include <moya/libmoya/Polygon.h>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/glm.hpp>

namespace {

v3d::moya::Vertex vertex(float x, float y, float z) {
    v3d::moya::Vertex v;
    v.point(glm::vec3(x, y, z));
    return v;
}

/**
 * The z = 0 plane, keeping the positive half space.
 **/
v3d::moya::Plane zPlane() {
    v3d::moya::Plane plane;
    plane.calculate(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    return plane;
}

};  // namespace

/**
 * The equation is the plane's only state, so a plane built from a normal and a point reports
 * back the normal and distance it was given rather than a second, separately written copy.
 **/
BOOST_AUTO_TEST_CASE(plane_representation_test) {
    v3d::moya::Plane plane;
    plane.calculate(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 5.0f, 0.0f));

    BOOST_TEST((plane.normal() == glm::vec3(0.0f, 1.0f, 0.0f)));
    BOOST_TEST(plane.distance() == 5.0f);
    BOOST_TEST(plane.distance(glm::vec3(0.0f, 7.0f, 0.0f)) == 2.0f);
}

/**
 * The three point constructor writes the same state the two argument calculate does. It used
 * to set only the stored normal, leaving classify reading an equation nobody had filled in.
 **/
BOOST_AUTO_TEST_CASE(plane_from_three_points_test) {
    v3d::moya::Plane plane(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(1.0f, 0.0f, 2.0f), glm::vec3(1.0f, 1.0f, 2.0f));

    BOOST_TEST(plane.classify(glm::vec3(0.0f, 0.0f, 5.0f)) == v3d::moya::Plane::POSITIVE);
    BOOST_TEST(plane.classify(glm::vec3(0.0f, 0.0f, 0.0f)) == v3d::moya::Plane::NEGATIVE);
    BOOST_TEST(plane.classify(glm::vec3(4.0f, 4.0f, 2.0f)) == v3d::moya::Plane::ON_PLANE);
}

/**
 * A ray hit is found from the same equation the classification is, so the two agree about
 * where the plane is.
 **/
BOOST_AUTO_TEST_CASE(plane_intersect_edge_test) {
    v3d::moya::Plane plane = zPlane();

    glm::vec3 hit;
    BOOST_TEST(plane.intersectEdge(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f), &hit));
    BOOST_TEST((hit == glm::vec3(0.0f, 0.0f, 0.0f)));

    // an edge that stays on one side is not crossed
    BOOST_TEST(!plane.intersectEdge(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 2.0f), &hit));
}

/**
 * The clip rewrites the polygon it was given. Handing the result back through a by-value
 * parameter is what made a clipped polygon come out unclipped.
 **/
BOOST_AUTO_TEST_CASE(plane_clip_rewrites_the_polygon_test) {
    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(0.0f, 0.0f, -1.0f));
    polygon->addVertex(vertex(1.0f, 0.0f, -1.0f));
    polygon->addVertex(vertex(1.0f, 0.0f, 1.0f));
    polygon->addVertex(vertex(0.0f, 0.0f, 1.0f));

    v3d::moya::Plane plane = zPlane();
    plane.clip(polygon);

    // the half of the quad below z = 0 is gone, and nothing that survived is below it
    BOOST_TEST(polygon->vertexCount() > 0u);
    for (size_t i = 0; i < polygon->vertexCount(); i++) {
        BOOST_TEST(polygon->vertex(i).point().z >= 0.0f);
    }
}

/**
 * A polygon wholly inside the kept half space survives the clip with its vertices.
 **/
BOOST_AUTO_TEST_CASE(plane_clip_keeps_an_inside_polygon_test) {
    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(0.0f, 0.0f, 1.0f));
    polygon->addVertex(vertex(1.0f, 0.0f, 1.0f));
    polygon->addVertex(vertex(1.0f, 1.0f, 2.0f));

    zPlane().clip(polygon);

    BOOST_TEST(polygon->vertexCount() == 3u);
}

/**
 * A polygon with no area has no inside to keep, and the walk opens on the vertex before the
 * first one - so it is left alone rather than indexed off the front.
 **/
BOOST_AUTO_TEST_CASE(plane_clip_degenerate_polygon_test) {
    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();

    zPlane().clip(polygon);
    BOOST_TEST(polygon->vertexCount() == 0u);

    polygon->addVertex(vertex(0.0f, 0.0f, 1.0f));
    polygon->addVertex(vertex(1.0f, 0.0f, 1.0f));

    zPlane().clip(polygon);
    BOOST_TEST(polygon->vertexCount() == 2u);
}

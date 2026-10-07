/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/geometry/Frustum.h>
#include <api/type/geometry/Plane.h>
#include <moya/libmoya/Polygon.h>

#include <cmath>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {

v3d::moya::Vertex vertex(float x, float y, float z) {
    v3d::moya::Vertex v;
    v.point(glm::vec3(x, y, z));
    return v;
}

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
 * The clip rewrites the polygon it is called on, so the polygon itself comes out clipped.
 **/
BOOST_AUTO_TEST_CASE(polygon_clip_rewrites_the_polygon_test) {
    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(0.0f, 0.0f, -1.0f));
    polygon->addVertex(vertex(1.0f, 0.0f, -1.0f));
    polygon->addVertex(vertex(1.0f, 0.0f, 1.0f));
    polygon->addVertex(vertex(0.0f, 0.0f, 1.0f));

    polygon->clip(zPlane());

    // the half of the quad below z = 0 is gone, and nothing that survived is below it
    BOOST_TEST(polygon->vertexCount() > 0u);
    for (size_t i = 0; i < polygon->vertexCount(); i++) {
        BOOST_TEST(polygon->vertex(i).point().z >= 0.0f);
    }
}

/**
 * A polygon wholly inside the kept half space survives the clip with its vertices.
 **/
BOOST_AUTO_TEST_CASE(polygon_clip_keeps_an_inside_polygon_test) {
    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(0.0f, 0.0f, 1.0f));
    polygon->addVertex(vertex(1.0f, 0.0f, 1.0f));
    polygon->addVertex(vertex(1.0f, 1.0f, 2.0f));

    polygon->clip(zPlane());

    BOOST_TEST(polygon->vertexCount() == 3u);
}

/**
 * A polygon with no area has no inside to keep, and the clip loop starts on the vertex before
 * the first one, so the polygon is left alone rather than indexed off the front.
 **/
BOOST_AUTO_TEST_CASE(polygon_clip_degenerate_polygon_test) {
    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();

    polygon->clip(zPlane());
    BOOST_TEST(polygon->vertexCount() == 0u);

    polygon->addVertex(vertex(0.0f, 0.0f, 1.0f));
    polygon->addVertex(vertex(1.0f, 0.0f, 1.0f));

    polygon->clip(zPlane());
    BOOST_TEST(polygon->vertexCount() == 2u);
}

/**
 * A frustum clip runs every plane over the polygon, so what survives is inside all six.
 **/
BOOST_AUTO_TEST_CASE(polygon_clip_frustum_test) {
    v3d::moya::Polygon polygon;
    polygon.addVertex(vertex(-3.0f, -3.0f, 0.0f));
    polygon.addVertex(vertex(3.0f, -3.0f, 0.0f));
    polygon.addVertex(vertex(0.0f, 3.0f, 0.0f));

    const v3d::type::geometry::Frustum frustum(glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f),
        v3d::type::geometry::Frustum::Depth::MinusOneToOne);
    v3d::moya::clip(polygon, frustum);

    BOOST_TEST(polygon.vertexCount() >= 3u);
    for (size_t i = 0; i < polygon.vertexCount(); i++) {
        const glm::vec3 point = polygon.vertex(i).point();
        BOOST_TEST(point.x >= -1.0001f);
        BOOST_TEST(point.x <= 1.0001f);
        BOOST_TEST(point.y >= -1.0001f);
        BOOST_TEST(point.y <= 1.0001f);
    }
}

/**
 * A vertex the clip makes where an edge crosses the plane has texture coordinates as far
 * along the edge as it is.
 **/
BOOST_AUTO_TEST_CASE(polygon_clip_carries_texture_coordinates_test) {
    v3d::moya::Polygon polygon;
    const glm::vec3 corners[3] = { glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 1.0f, 1.0f) };
    const glm::vec2 st[3] = { glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 1.0f) };
    for (unsigned int k = 0; k < 3; k++) {
        v3d::moya::Vertex v = vertex(corners[k].x, corners[k].y, corners[k].z);
        v.st(st[k]);
        polygon.addVertex(v);
    }

    polygon.clip(zPlane());

    BOOST_REQUIRE(polygon.vertexCount() > 0u);
    for (size_t i = 0; i < polygon.vertexCount(); i++) {
        const v3d::moya::Vertex & v = polygon.vertex(i);
        BOOST_TEST_CONTEXT("vertex " << i) {
            BOOST_REQUIRE(v.hasTexCoord());
            // every vertex on the plane is half way along an edge from the first corner
            if (std::fabs(v.point().z) < 1.0e-5f) {
                BOOST_CHECK_CLOSE(v.st().x + v.st().y, 0.5f, 0.01f);
            }
        }
    }
}

/**
 * A vertex the clip makes where an edge crosses the plane has the colour and the shading
 * normal as far along the edge as it is, the normal at unit length. An edge with an end that
 * has no normal gives the crossing none, so it is filled from the primitive later.
 **/
BOOST_AUTO_TEST_CASE(polygon_clip_carries_colour_and_normal_test) {
    v3d::moya::Polygon polygon;
    const glm::vec3 corners[3] = { glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 1.0f, 1.0f) };
    const glm::vec3 colours[3] = { glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f) };
    const glm::vec3 normals[3] = { glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f) };
    for (unsigned int k = 0; k < 3; k++) {
        v3d::moya::Vertex v = vertex(corners[k].x, corners[k].y, corners[k].z);
        v.color(colours[k]);
        // the second corner is left without a normal
        if (k != 1) {
            v.normal(normals[k]);
        }
        polygon.addVertex(v);
    }

    polygon.clip(zPlane());

    // the first corner is clipped away, and each of its two edges is cut half way along
    unsigned int found = 0;
    for (size_t i = 0; i < polygon.vertexCount(); i++) {
        const v3d::moya::Vertex & v = polygon.vertex(i);
        if (std::fabs(v.point().z) > 1.0e-4f) {
            continue;
        }
        BOOST_TEST_CONTEXT("vertex " << i) {
            BOOST_REQUIRE(v.hasColor());
            if (v.point().x > 0.25f) {
                // on the edge to the second corner, which has no normal
                BOOST_TEST(v.color().x == 0.5f, boost::test_tools::tolerance(0.0001f));
                BOOST_TEST(v.color().y == 0.5f, boost::test_tools::tolerance(0.0001f));
                BOOST_CHECK_SMALL(v.color().z, 1.0e-4f);
                BOOST_TEST(!v.hasNormal());
            } else {
                // on the edge to the third corner
                BOOST_TEST(v.color().x == 0.5f, boost::test_tools::tolerance(0.0001f));
                BOOST_CHECK_SMALL(v.color().y, 1.0e-4f);
                BOOST_TEST(v.color().z == 0.5f, boost::test_tools::tolerance(0.0001f));
                BOOST_REQUIRE(v.hasNormal());
                const float half = std::sqrt(0.5f);
                BOOST_CHECK_SMALL(v.normal().x, 1.0e-4f);
                BOOST_TEST(v.normal().y == half, boost::test_tools::tolerance(0.0001f));
                BOOST_TEST(v.normal().z == half, boost::test_tools::tolerance(0.0001f));
            }
            found++;
        }
    }
    BOOST_TEST(found == 2u);
}

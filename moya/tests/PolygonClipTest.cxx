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
    plane.calculate(glm::vec3(0.0F, 0.0F, 1.0F), glm::vec3(0.0F, 0.0F, 0.0F));
    return plane;
}

/**
 * A plane and an edge on which the two plane tests disagree: classify() puts s on the cut side
 * and p on the kept side, and intersectEdge() finds no meeting, because p is a hair past the
 * plane and the parameter it computes for p comes out just above one.
 **/
v3d::type::geometry::Plane tiltedPlane() {
    v3d::type::geometry::Plane plane;
    plane.set(glm::vec3(-0.522197902F, 0.622982621F, 0.582410455F), 0.0900339484F);
    return plane;
}

glm::vec3 cutEnd() {
    return glm::vec3(1.35210562F, -1.14019823F, 2.00327873F);
}

glm::vec3 keptEnd() {
    return glm::vec3(-3.20165992F, -0.251474023F, -2.44707513F);
}

/**
 * A polygon whose split by one plane a test can call, which the renderer reaches only through
 * the split into four.
 **/
class Splittable final : public v3d::moya::Polygon {
 public:
    using v3d::moya::Polygon::split;
};

/**
 * A triangle with that edge, and a third corner well inside the kept half space.
 **/
boost::shared_ptr<Splittable> tiltedTriangle() {
    const glm::vec3 inside = keptEnd() + glm::vec3(-0.522197902F, 0.622982621F, 0.582410455F) * 2.0F;
    boost::shared_ptr<Splittable> polygon = boost::make_shared<Splittable>();
    polygon->addVertex(vertex(cutEnd().x, cutEnd().y, cutEnd().z));
    polygon->addVertex(vertex(keptEnd().x, keptEnd().y, keptEnd().z));
    polygon->addVertex(vertex(inside.x, inside.y, inside.z));
    return polygon;
}

/**
 * An edge the two plane tests disagree on still crosses the plane, at its kept end. The vertex
 * made for it is not the point left over from the edge clipped before it.
 **/
BOOST_AUTO_TEST_CASE(polygon_clip_an_edge_that_ends_past_the_plane_test) {
    const v3d::type::geometry::Plane plane = tiltedPlane();
    BOOST_REQUIRE(plane.classify(cutEnd()) == v3d::type::geometry::Plane::NEGATIVE);
    BOOST_REQUIRE(plane.classify(keptEnd()) == v3d::type::geometry::Plane::POSITIVE);
    glm::vec3 hit;
    BOOST_REQUIRE(!plane.intersectEdge(cutEnd(), keptEnd(), &hit));

    const boost::shared_ptr<Splittable> polygon = tiltedTriangle();
    polygon->clip(plane);

    // the crossing of the third edge, the crossing of this one, its kept end, the third corner
    BOOST_REQUIRE_EQUAL(polygon->vertexCount(), 4U);
    BOOST_TEST(glm::distance(polygon->vertex(1).point(), keptEnd()) < 1.0e-4F);
}

/**
 * Split takes the same edge as a crossing too, rather than as an edge wholly on one side, so
 * each piece holds only what is on its own side of the plane.
 **/
BOOST_AUTO_TEST_CASE(polygon_split_an_edge_that_ends_past_the_plane_test) {
    const v3d::type::geometry::Plane plane = tiltedPlane();
    const boost::shared_ptr<Splittable> polygon = tiltedTriangle();
    const boost::shared_ptr<v3d::moya::Polygon> negative = boost::make_shared<v3d::moya::Polygon>();
    const boost::shared_ptr<v3d::moya::Polygon> positive = boost::make_shared<v3d::moya::Polygon>();

    polygon->split(plane, negative, positive);

    BOOST_TEST(negative->vertexCount() >= 3U);
    BOOST_TEST(positive->vertexCount() >= 3U);
    for (size_t i = 0; i < negative->vertexCount(); i++) {
        BOOST_TEST(plane.signedDistance(negative->vertex(i).point()) <= 1.0e-4F);
    }
    for (size_t i = 0; i < positive->vertexCount(); i++) {
        BOOST_TEST(plane.signedDistance(positive->vertex(i).point()) >= -1.0e-4F);
    }
}

/**
 * The clip rewrites the polygon it is called on, so the polygon itself comes out clipped.
 **/
BOOST_AUTO_TEST_CASE(polygon_clip_rewrites_the_polygon_test) {
    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(0.0F, 0.0F, -1.0F));
    polygon->addVertex(vertex(1.0F, 0.0F, -1.0F));
    polygon->addVertex(vertex(1.0F, 0.0F, 1.0F));
    polygon->addVertex(vertex(0.0F, 0.0F, 1.0F));

    polygon->clip(zPlane());

    // the half of the quad below z = 0 is gone, and nothing that survived is below it
    BOOST_TEST(polygon->vertexCount() > 0U);
    for (size_t i = 0; i < polygon->vertexCount(); i++) {
        BOOST_TEST(polygon->vertex(i).point().z >= 0.0F);
    }
}

/**
 * A polygon wholly inside the kept half space survives the clip with its vertices.
 **/
BOOST_AUTO_TEST_CASE(polygon_clip_keeps_an_inside_polygon_test) {
    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(0.0F, 0.0F, 1.0F));
    polygon->addVertex(vertex(1.0F, 0.0F, 1.0F));
    polygon->addVertex(vertex(1.0F, 1.0F, 2.0F));

    polygon->clip(zPlane());

    BOOST_TEST(polygon->vertexCount() == 3U);
}

/**
 * A polygon with no area has no inside to keep, and the clip loop starts on the vertex before
 * the first one, so the polygon is left alone rather than indexed off the front.
 **/
BOOST_AUTO_TEST_CASE(polygon_clip_degenerate_polygon_test) {
    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();

    polygon->clip(zPlane());
    BOOST_TEST(polygon->vertexCount() == 0U);

    polygon->addVertex(vertex(0.0F, 0.0F, 1.0F));
    polygon->addVertex(vertex(1.0F, 0.0F, 1.0F));

    polygon->clip(zPlane());
    BOOST_TEST(polygon->vertexCount() == 2U);
}

/**
 * A frustum clip runs every plane over the polygon, so what survives is inside all six.
 **/
BOOST_AUTO_TEST_CASE(polygon_clip_frustum_test) {
    v3d::moya::Polygon polygon;
    polygon.addVertex(vertex(-3.0F, -3.0F, 0.0F));
    polygon.addVertex(vertex(3.0F, -3.0F, 0.0F));
    polygon.addVertex(vertex(0.0F, 3.0F, 0.0F));

    const v3d::type::geometry::Frustum frustum(glm::ortho(-1.0F, 1.0F, -1.0F, 1.0F, -1.0F, 1.0F),
        v3d::type::geometry::Frustum::Depth::MinusOneToOne);
    v3d::moya::clip(polygon, frustum);

    BOOST_TEST(polygon.vertexCount() >= 3U);
    for (size_t i = 0; i < polygon.vertexCount(); i++) {
        const glm::vec3 point = polygon.vertex(i).point();
        BOOST_TEST(point.x >= -1.0001F);
        BOOST_TEST(point.x <= 1.0001F);
        BOOST_TEST(point.y >= -1.0001F);
        BOOST_TEST(point.y <= 1.0001F);
    }
}

/**
 * A vertex the clip makes where an edge crosses the plane has texture coordinates as far
 * along the edge as it is.
 **/
BOOST_AUTO_TEST_CASE(polygon_clip_carries_texture_coordinates_test) {
    v3d::moya::Polygon polygon;
    const glm::vec3 corners[3] = { glm::vec3(0.0F, 0.0F, -1.0F), glm::vec3(1.0F, 0.0F, 1.0F),
        glm::vec3(0.0F, 1.0F, 1.0F) };
    const glm::vec2 st[3] = { glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 0.0F), glm::vec2(0.0F, 1.0F) };
    for (unsigned int k = 0; k < 3; k++) {
        v3d::moya::Vertex v = vertex(corners[k].x, corners[k].y, corners[k].z);
        v.st(st[k]);
        polygon.addVertex(v);
    }

    polygon.clip(zPlane());

    BOOST_REQUIRE(polygon.vertexCount() > 0U);
    for (size_t i = 0; i < polygon.vertexCount(); i++) {
        const v3d::moya::Vertex & v = polygon.vertex(i);
        BOOST_TEST_CONTEXT("vertex " << i) {
            BOOST_REQUIRE(v.hasTexCoord());
            // every vertex on the plane is half way along an edge from the first corner
            if (std::fabs(v.point().z) < 1.0e-5F) {
                BOOST_CHECK_CLOSE(v.st().x + v.st().y, 0.5F, 0.01F);
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
    const glm::vec3 corners[3] = { glm::vec3(0.0F, 0.0F, -1.0F), glm::vec3(1.0F, 0.0F, 1.0F),
        glm::vec3(0.0F, 1.0F, 1.0F) };
    const glm::vec3 colours[3] = { glm::vec3(1.0F, 0.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F),
        glm::vec3(0.0F, 0.0F, 1.0F) };
    const glm::vec3 normals[3] = { glm::vec3(0.0F, 0.0F, 1.0F), glm::vec3(1.0F, 0.0F, 0.0F),
        glm::vec3(0.0F, 1.0F, 0.0F) };
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
        if (std::fabs(v.point().z) > 1.0e-4F) {
            continue;
        }
        BOOST_TEST_CONTEXT("vertex " << i) {
            BOOST_REQUIRE(v.hasColor());
            if (v.point().x > 0.25F) {
                // on the edge to the second corner, which has no normal
                BOOST_TEST(v.color().x == 0.5F, boost::test_tools::tolerance(0.0001F));
                BOOST_TEST(v.color().y == 0.5F, boost::test_tools::tolerance(0.0001F));
                BOOST_CHECK_SMALL(v.color().z, 1.0e-4F);
                BOOST_TEST(!v.hasNormal());
            } else {
                // on the edge to the third corner
                BOOST_TEST(v.color().x == 0.5F, boost::test_tools::tolerance(0.0001F));
                BOOST_CHECK_SMALL(v.color().y, 1.0e-4F);
                BOOST_TEST(v.color().z == 0.5F, boost::test_tools::tolerance(0.0001F));
                BOOST_REQUIRE(v.hasNormal());
                const float half = std::sqrt(0.5F);
                BOOST_CHECK_SMALL(v.normal().x, 1.0e-4F);
                BOOST_TEST(v.normal().y == half, boost::test_tools::tolerance(0.0001F));
                BOOST_TEST(v.normal().z == half, boost::test_tools::tolerance(0.0001F));
            }
            found++;
        }
    }
    BOOST_TEST(found == 2U);
}

};  // namespace

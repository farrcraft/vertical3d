/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/rib/Parameters.h>
#include <moya/libmoya/Polygon.h>
#include <moya/libmoya/RenderContext.h>
#include <moya/libmoya/Vertex.h>
#include <moya/libmoya/reyes/MicroPolygonGrid.h>

#include <cstddef>
#include <string>
#include <vector>

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
 * The defaults a context starts on, which stand in for the RiFormat and bucketing options a
 * RIB file would otherwise set.
 **/
BOOST_AUTO_TEST_CASE(render_context_defaults_test) {
    const v3d::moya::RenderContext rc;

    BOOST_TEST(rc.bucketWidth() == 16U);
    BOOST_TEST(rc.bucketHeight() == 16U);
    BOOST_TEST(rc.gridSize() == 256U);
    BOOST_TEST(rc.shadingRate() == 1.0F);
    BOOST_TEST(rc.imageWidth() == 320U);
    BOOST_TEST(rc.imageHeight() == 240U);
    BOOST_TEST(rc.pixelAspect() == 1.0F);
}

BOOST_AUTO_TEST_CASE(render_context_image_resolution_test) {
    v3d::moya::RenderContext rc;

    rc.imageResolution({1024, 768}, 1.3F);

    BOOST_TEST(rc.imageWidth() == 1024U);
    BOOST_TEST(rc.imageHeight() == 768U);
    BOOST_TEST(rc.pixelAspect() == 1.3F);
}

/**
 * A named context takes the same defaults as an unnamed one. The name is what RiBegin is
 * given, and is not itself an option.
 **/
BOOST_AUTO_TEST_CASE(render_context_named_test) {
    const v3d::moya::RenderContext rc(std::string("scene.rib"));

    BOOST_TEST(rc.imageWidth() == 320U);
    BOOST_TEST(rc.gridSize() == 256U);
}

/**
 * Each reserved coordinate system is identity until something saves over it.
 **/
BOOST_AUTO_TEST_CASE(render_context_reserved_coordinate_systems_test) {
    v3d::moya::RenderContext rc;
    const glm::mat4x4 identity(1.0F);

    const char* reserved[] = { "object", "world", "camera", "screen", "raster", "NDC" };
    for (const auto* name : reserved) {
        BOOST_TEST((rc.coordinateSystem(name) == identity));
    }
}

/**
 * saveCoordinateSystem files the current transform under a name, and setCoordinateSystem
 * restores it. Together they implement RiCoordinateSystem and RiCoordSysTransform.
 **/
BOOST_AUTO_TEST_CASE(render_context_coordinate_system_test) {
    v3d::moya::RenderContext rc;
    const glm::mat4x4 scaled(3.0F);

    rc.setTransform(scaled);
    rc.saveCoordinateSystem("world");
    BOOST_TEST((rc.coordinateSystem("world") == scaled));

    rc.setIdentityTransform();
    rc.saveCoordinateSystem("world");
    BOOST_TEST((rc.coordinateSystem("world") == glm::mat4x4(1.0F)));

    rc.setCoordinateSystem("world");
    rc.saveCoordinateSystem("object");
    BOOST_TEST((rc.coordinateSystem("object") == glm::mat4x4(1.0F)));
}

/**
 * A translate composes onto the current transform rather than replacing it, so two of them
 * accumulate.
 **/
BOOST_AUTO_TEST_CASE(render_context_translate_test) {
    v3d::moya::RenderContext rc;

    rc.setIdentityTransform();
    rc.translate(1.0F, 2.0F, 3.0F);
    rc.translate(1.0F, 2.0F, 3.0F);
    rc.saveCoordinateSystem("object");

    glm::mat4x4 composed = rc.coordinateSystem("object");
    BOOST_TEST((glm::vec3(composed[3]) == glm::vec3(2.0F, 4.0F, 6.0F)));
}

/**
 * A rotate composes onto the current transform the way a translate does.
 **/
BOOST_AUTO_TEST_CASE(render_context_rotate_test) {
    v3d::moya::RenderContext rc;

    rc.setIdentityTransform();
    rc.rotate(90.0F, {0.0F, 0.0F, 1.0F});
    rc.saveCoordinateSystem("object");

    // the angle is in degrees, as RiRotate takes it
    const glm::mat4x4 composed = rc.coordinateSystem("object");
    glm::vec3 turned = glm::vec3(composed * glm::vec4(1.0F, 0.0F, 0.0F, 1.0F));
    BOOST_TEST(turned.x == 0.0F, boost::test_tools::tolerance(0.0001F));
    BOOST_TEST(turned.y == 1.0F, boost::test_tools::tolerance(0.0001F));
}

BOOST_AUTO_TEST_CASE(render_context_scale_test) {
    v3d::moya::RenderContext rc;

    rc.setIdentityTransform();
    rc.scale(2.0F, 3.0F, 4.0F);
    rc.saveCoordinateSystem("object");

    const glm::mat4x4 composed = rc.coordinateSystem("object");
    const glm::vec3 scaled = glm::vec3(composed * glm::vec4(1.0F, 1.0F, 1.0F, 1.0F));
    BOOST_TEST((scaled == glm::vec3(2.0F, 3.0F, 4.0F)));
}

/**
 * addPolygon() files a polygon in the bucket that holds the upper left corner of its raster
 * bound.
 **/
BOOST_AUTO_TEST_CASE(render_context_buckets_a_polygon_test) {
    v3d::moya::RenderContext rc;
    rc.prepareWorld();

    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.9F, -0.9F, 5.0F));
    polygon->addVertex(vertex(0.9F, -0.9F, 5.0F));
    polygon->addVertex(vertex(0.9F, 0.9F, 5.0F));
    polygon->addVertex(vertex(-0.9F, 0.9F, 5.0F));
    rc.addPolygon(polygon);

    BOOST_TEST(rc.framebuffer()->primitiveCount() == 1U);
}

/**
 * A polygon far larger than one grid is undiceable. Rendering the buckets splits it, and hands
 * each piece back to addPolygon(), which measures it in turn. The recursion ends because a
 * piece that is not smaller than its parent is not handed back. Without that rule, the pieces
 * would be split again without end.
 **/
BOOST_AUTO_TEST_CASE(render_context_split_terminates_test) {
    v3d::moya::RenderContext rc;
    rc.prepareWorld();

    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.9F, -0.9F, 5.0F));
    polygon->addVertex(vertex(0.9F, -0.9F, 5.0F));
    polygon->addVertex(vertex(0.9F, 0.9F, 5.0F));
    polygon->addVertex(vertex(-0.9F, 0.9F, 5.0F));
    rc.addPolygon(polygon);

    rc.render();

    /*
        The default screen window is [-4/3, 4/3] by [-1, 1] over a 320 by 240 image, so the
        raster bound is 216 pixels across. A grid covers 16, so four rounds of four way
        splitting bring every piece under one: 4^4 pieces, all diceable and bucketed.
    */
    BOOST_TEST(rc.framebuffer()->primitiveCount() == 256U);
}

namespace {

/**
 * A triangle small enough that addPolygon() measures it as diceable, so its vertices are
 * moved into eye space and it is not split. Its plane is (1, 1, 0) normalised, a normal that
 * a scale of the x or y axis alone changes.
 **/
boost::shared_ptr<v3d::moya::Polygon> diagonal() {
    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(0.01F, 0.0F, 5.0F));
    polygon->addVertex(vertex(0.0F, 0.01F, 5.0F));
    polygon->addVertex(vertex(0.0F, 0.01F, 5.01F));
    return polygon;
}

};  // namespace

/**
 * A polygon that says nothing about its normals gets its own plane on every vertex, as both Ng
 * and N, so such a surface is faceted.
 **/
BOOST_AUTO_TEST_CASE(render_context_face_normal_test) {
    v3d::moya::RenderContext rc;
    rc.prepareWorld();

    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.01F, -0.01F, 5.0F));
    polygon->addVertex(vertex(0.01F, -0.01F, 5.0F));
    polygon->addVertex(vertex(0.01F, 0.01F, 5.0F));
    // nothing has written a shading normal yet, so addPolygon() fills one in
    BOOST_TEST(!polygon->vertex(0).hasNormal());

    rc.addPolygon(polygon);

    BOOST_REQUIRE(polygon->diceable());
    for (size_t i = 0; i < polygon->vertexCount(); i++) {
        BOOST_TEST((polygon->vertex(i).geometricNormal() == glm::vec3(0.0F, 0.0F, 1.0F)));
        BOOST_TEST((polygon->vertex(i).normal() == glm::vec3(0.0F, 0.0F, 1.0F)));
    }
}

/**
 * A varying "N" is the shading normal and overrides the plane. Ng stays the plane, because SL
 * defines faceforward() and calculatenormal() in terms of the pair.
 **/
BOOST_AUTO_TEST_CASE(render_context_normal_override_test) {
    v3d::moya::RenderContext rc;
    rc.prepareWorld();

    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    for (int i = 0; i < 3; i++) {
        v3d::moya::Vertex v = vertex(i == 1 ? 0.01F : -0.01F, i == 2 ? 0.01F : -0.01F, 5.0F);
        v.normal(glm::vec3(0.0F, 1.0F, 0.0F));
        polygon->addVertex(v);
    }
    rc.addPolygon(polygon);

    BOOST_REQUIRE(polygon->diceable());
    for (size_t i = 0; i < polygon->vertexCount(); i++) {
        BOOST_TEST((polygon->vertex(i).normal() == glm::vec3(0.0F, 1.0F, 0.0F)));
        BOOST_TEST((polygon->vertex(i).geometricNormal() == glm::vec3(0.0F, 0.0F, 1.0F)));
    }
}

/**
 * A normal transforms by the inverse transpose. Under a rotation or a uniform scale, that
 * moves a normal the way the matrix that moves the points does. Only a scale of one axis
 * tells the two apart, and then the normal of the transformed plane leans the opposite way to
 * the points.
 **/
BOOST_AUTO_TEST_CASE(render_context_normal_inverse_transpose_test) {
    v3d::moya::RenderContext rc;
    rc.prepareWorld();
    rc.scale(1.0F, 2.0F, 1.0F);

    const boost::shared_ptr<v3d::moya::Polygon> polygon = diagonal();
    rc.addPolygon(polygon);

    BOOST_REQUIRE(polygon->diceable());
    // the object plane is (1, 1, 0) normalised. The inverse transpose of a scale of two in y
    // halves the y component, and gives the plane the scaled points lie in
    const glm::vec3 expected = glm::normalize(glm::vec3(1.0F, 0.5F, 0.0F));
    for (size_t i = 0; i < polygon->vertexCount(); i++) {
        const glm::vec3 normal = polygon->vertex(i).geometricNormal();
        BOOST_TEST(normal.x == expected.x, boost::test_tools::tolerance(0.0001F));
        BOOST_TEST(normal.y == expected.y, boost::test_tools::tolerance(0.0001F));
        BOOST_TEST(normal.z == expected.z, boost::test_tools::tolerance(0.0001F));
    }

    // and the transformed vertices agree: the plane the normal names is the plane they are in
    const glm::vec3 p0 = polygon->vertex(0).point();
    const glm::vec3 p1 = polygon->vertex(1).point();
    const glm::vec3 p2 = polygon->vertex(2).point();
    const glm::vec3 measured = glm::normalize(glm::cross(p1 - p0, p2 - p0));
    BOOST_TEST(measured.x == expected.x, boost::test_tools::tolerance(0.0001F));
    BOOST_TEST(measured.y == expected.y, boost::test_tools::tolerance(0.0001F));
}

/**
 * Dicing interpolates the shading normal onto the grid the way it interpolates the position and
 * the colour, so a surface given a varying "N" comes out smooth. The geometric
 * normal is one value across the primitive and is copied rather than interpolated.
 **/
BOOST_AUTO_TEST_CASE(render_context_dice_interpolates_normal_test) {
    v3d::moya::RenderContext rc;
    rc.prepareWorld();

    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    const glm::vec3 corners[4] = {
        glm::vec3(-0.01F, -0.01F, 5.0F), glm::vec3(0.01F, -0.01F, 5.0F),
        glm::vec3(0.01F, 0.01F, 5.0F), glm::vec3(-0.01F, 0.01F, 5.0F)
    };
    for (int i = 0; i < 4; i++) {
        v3d::moya::Vertex v;
        v.point(corners[i]);
        // the first corner leans one way and the rest face front, so a midpoint of the grid
        // has to be between the two rather than either
        v.normal(i == 0 ? glm::normalize(glm::vec3(1.0F, 0.0F, 1.0F)) : glm::vec3(0.0F, 0.0F, 1.0F));
        polygon->addVertex(v);
    }
    rc.addPolygon(polygon);
    BOOST_REQUIRE(polygon->diceable());

    boost::shared_ptr<v3d::moya::reyes::MicroPolygonGrid> grid;
    BOOST_REQUIRE(polygon->dice(grid, rc));
    BOOST_REQUIRE(grid);

    const unsigned int last = grid->size() - 1;
    // the corner that leans keeps its own normal, and one diagonally across from it does not
    BOOST_TEST(grid->vertex(0, 0).normal().x > 0.7F);
    BOOST_TEST(grid->vertex(last, last).normal().x == 0.0F, boost::test_tools::tolerance(0.0001F));
    // between them the normal is neither, and is still a unit vector
    const glm::vec3 middle = grid->vertex(last / 2, last / 2).normal();
    BOOST_TEST(middle.x > 0.0F);
    BOOST_TEST(middle.x < 0.7F);
    BOOST_TEST(glm::length(middle) == 1.0F, boost::test_tools::tolerance(0.0001F));

    // Ng is the plane, everywhere on the grid
    BOOST_TEST((grid->vertex(0, 0).geometricNormal() == glm::vec3(0.0F, 0.0F, 1.0F)));
    BOOST_TEST((grid->vertex(last, last).geometricNormal() == glm::vec3(0.0F, 0.0F, 1.0F)));
}

/**
 * A piece takes the whole primitive's plane through place(), the same route the colour takes.
 * The plane is the piece's geometric normal, and it fills any vertex of the piece that has no
 * shading normal of its own.
 **/
BOOST_AUTO_TEST_CASE(render_context_split_carries_the_normal_test) {
    v3d::moya::RenderContext rc;
    rc.prepareWorld();

    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.9F, -0.9F, 5.0F));
    polygon->addVertex(vertex(0.9F, -0.9F, 5.0F));
    polygon->addVertex(vertex(0.9F, 0.9F, 5.0F));
    polygon->addVertex(vertex(-0.9F, 0.9F, 5.0F));
    rc.addPolygon(polygon);

    // too large for one grid, so rendering the buckets splits it and does not dice it. The
    // plane it hands its pieces is the one it was placed with
    BOOST_REQUIRE(!polygon->diceable());
    BOOST_TEST((polygon->normal() == glm::vec3(0.0F, 0.0F, 1.0F)));

    // the pieces reach addPolygon() again already placed, so they keep that plane. The render
    // ends with every piece bucketed. It would not if a piece were measured with the current
    // state instead of its parent's
    rc.render();
    BOOST_TEST(rc.framebuffer()->primitiveCount() == 256U);
}

/**
 * An imager runs after the last bucket, over the finished frame, and gives a pixel nothing
 * was drawn into the value the scene set for it.
 *
 * render() runs the imager the same way after either hider. The imager reads the coverage
 * plane, because without it a pixel the hider never reached and a black one hold the same
 * value.
 **/
BOOST_AUTO_TEST_CASE(rendercontext_imager_test) {
    v3d::moya::RenderContext rc;
    rc.imageResolution({64, 48}, 1.0F);
    rc.clipping({.hither = 1.0F, .yon = 100.0F});
    rc.prepareWorld();

    v3d::render::offline::rib::ParameterList list;
    list.add("background",
        v3d::render::offline::rib::Declaration(
            v3d::render::offline::rib::Declaration::Storage::UNIFORM,
            v3d::render::offline::rib::Declaration::Type::COLOR, 1),
        { 0.15F, 0.25F, 0.45F }, std::vector<std::string>());
    rc.imager("background", list);

    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.7F, -0.2F, 5.0F));
    polygon->addVertex(vertex(0.3F, -0.2F, 5.0F));
    polygon->addVertex(vertex(0.3F, 0.8F, 5.0F));
    polygon->addVertex(vertex(-0.7F, 0.8F, 5.0F));
    rc.addPolygon(polygon);
    rc.render();

    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = rc.framebuffer()->planes();
    // the quad covers raster x over [15.2, 39.2] and y over [4.8, 28.8], so this is inside
    // it and keeps the colour the hider wrote
    BOOST_CHECK_CLOSE(planes->value(v3d::moya::FrameBuffer::RED, 24, 16), 1.0F, 0.01F);
    // and this is outside it, where the imager is the whole of the pixel
    BOOST_CHECK_CLOSE(planes->value(v3d::moya::FrameBuffer::RED, 55, 40), 0.15F, 0.01F);
    BOOST_CHECK_CLOSE(planes->value(v3d::moya::FrameBuffer::BLUE, 55, 40), 0.45F, 0.01F);
    BOOST_CHECK_CLOSE(planes->value(v3d::moya::FrameBuffer::COVERAGE, 55, 40), 1.0F, 0.01F);
}

/**
 * A coordinate system that is neither predefined nor saved is refused. Restoring it leaves the
 * current transformation as it was, and reading it gives the identity rather than adding a
 * matrix of zeros.
 **/
BOOST_AUTO_TEST_CASE(render_context_unknown_coordinate_system_test) {
    v3d::moya::RenderContext rc("");
    rc.scale(2.0F, 2.0F, 2.0F);
    rc.setCoordinateSystem("nowhere");
    rc.saveCoordinateSystem("kept");

    BOOST_TEST((rc.coordinateSystem("kept") == glm::scale(glm::mat4x4(1.0F), glm::vec3(2.0F))));
    BOOST_TEST((rc.coordinateSystem("nowhere") == glm::mat4x4(1.0F)));
}

/**
 * An unknown projection is refused, and the projection named before it stays in force.
 **/
BOOST_AUTO_TEST_CASE(render_context_unknown_projection_test) {
    v3d::moya::RenderContext rc("");
    rc.projection("perspective", 45.0F);
    const glm::mat4x4 screen = rc.coordinateSystem("screen");

    rc.projection("fisheye");
    BOOST_TEST(rc.perspective());
    BOOST_TEST((rc.coordinateSystem("screen") == screen));
}

};  // namespace

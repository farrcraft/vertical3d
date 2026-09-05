/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/glm.hpp>

#include "../libmoya/RenderContext.h"

namespace {

    v3d::moya::Vertex vertex(float x, float y, float z) {
        v3d::moya::Vertex v;
        v.point(glm::vec3(x, y, z));
        return v;
    }

};  // namespace

/**
 * The defaults a context starts on, which stand in for the RiFormat and bucketing options a
 * RIB file would otherwise set.
 **/
BOOST_AUTO_TEST_CASE(render_context_defaults_test) {
    v3d::moya::RenderContext rc;

    BOOST_TEST(rc.bucketWidth() == 16u);
    BOOST_TEST(rc.bucketHeight() == 16u);
    BOOST_TEST(rc.gridSize() == 256u);
    BOOST_TEST(rc.shadingRate() == 1.0f);
    BOOST_TEST(rc.imageWidth() == 320u);
    BOOST_TEST(rc.imageHeight() == 240u);
    BOOST_TEST(rc.pixelAspect() == 1.0f);
}

BOOST_AUTO_TEST_CASE(render_context_image_resolution_test) {
    v3d::moya::RenderContext rc;

    rc.imageResolution(1024, 768, 1.3f);

    BOOST_TEST(rc.imageWidth() == 1024u);
    BOOST_TEST(rc.imageHeight() == 768u);
    BOOST_TEST(rc.pixelAspect() == 1.3f);
}

/**
 * A named context takes the same defaults as an unnamed one - the name is what RiBegin is
 * given and is not itself an option.
 **/
BOOST_AUTO_TEST_CASE(render_context_named_test) {
    v3d::moya::RenderContext rc(std::string("scene.rib"));

    BOOST_TEST(rc.imageWidth() == 320u);
    BOOST_TEST(rc.gridSize() == 256u);
}

/**
 * The six reserved systems are identity until something saves over them, so a lookup before
 * any transform has been set does not hand back an uninitialised matrix.
 **/
BOOST_AUTO_TEST_CASE(render_context_reserved_coordinate_systems_test) {
    v3d::moya::RenderContext rc;
    glm::mat4x4 identity(1.0f);

    const char* reserved[] = { "object", "world", "camera", "screen", "raster", "NDC" };
    for (auto name : reserved) {
        BOOST_TEST((rc.coordinateSystem(name) == identity));
    }
}

/**
 * saveCoordinateSystem files the current transform under a name, and setCoordinateSystem is
 * the way back - together they are what RiCoordinateSystem and RiCoordSysTransform do.
 **/
BOOST_AUTO_TEST_CASE(render_context_coordinate_system_test) {
    v3d::moya::RenderContext rc;
    glm::mat4x4 scaled(3.0f);

    rc.setTransform(scaled);
    rc.saveCoordinateSystem("world");
    BOOST_TEST((rc.coordinateSystem("world") == scaled));

    rc.setIdentityTransform();
    rc.saveCoordinateSystem("world");
    BOOST_TEST((rc.coordinateSystem("world") == glm::mat4x4(1.0f)));

    rc.setCoordinateSystem("world");
    rc.saveCoordinateSystem("object");
    BOOST_TEST((rc.coordinateSystem("object") == glm::mat4x4(1.0f)));
}

/**
 * A translate composes onto the current transform rather than replacing it, so two of them
 * accumulate.
 **/
BOOST_AUTO_TEST_CASE(render_context_translate_test) {
    v3d::moya::RenderContext rc;

    rc.setIdentityTransform();
    rc.translate(1.0f, 2.0f, 3.0f);
    rc.translate(1.0f, 2.0f, 3.0f);
    rc.saveCoordinateSystem("object");

    glm::mat4x4 composed = rc.coordinateSystem("object");
    BOOST_TEST((glm::vec3(composed[3]) == glm::vec3(2.0f, 4.0f, 6.0f)));
}

/**
 * A rotate composes onto the current transform the way a translate does. Both it and scale
 * used to have empty bodies, so RiRotate and RiScale were silent no-ops.
 **/
BOOST_AUTO_TEST_CASE(render_context_rotate_test) {
    v3d::moya::RenderContext rc;

    rc.setIdentityTransform();
    rc.rotate(90.0f, 0.0f, 0.0f, 1.0f);
    rc.saveCoordinateSystem("object");

    // the angle is in degrees, which is what RiRotate states it in
    glm::mat4x4 composed = rc.coordinateSystem("object");
    glm::vec3 turned = glm::vec3(composed * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    BOOST_TEST(turned.x == 0.0f, boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(turned.y == 1.0f, boost::test_tools::tolerance(0.0001f));
}

BOOST_AUTO_TEST_CASE(render_context_scale_test) {
    v3d::moya::RenderContext rc;

    rc.setIdentityTransform();
    rc.scale(2.0f, 3.0f, 4.0f);
    rc.saveCoordinateSystem("object");

    glm::mat4x4 composed = rc.coordinateSystem("object");
    glm::vec3 scaled = glm::vec3(composed * glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    BOOST_TEST((scaled == glm::vec3(2.0f, 3.0f, 4.0f)));
}

/**
 * The first pass files a polygon in the bucket its raster bound opens in.
 **/
BOOST_AUTO_TEST_CASE(render_context_buckets_a_polygon_test) {
    v3d::moya::RenderContext rc;
    rc.prepareWorld();

    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.9f, -0.9f, 5.0f));
    polygon->addVertex(vertex(0.9f, -0.9f, 5.0f));
    polygon->addVertex(vertex(0.9f, 0.9f, 5.0f));
    polygon->addVertex(vertex(-0.9f, 0.9f, 5.0f));
    rc.addPolygon(polygon);

    BOOST_TEST(rc.framebuffer()->primitiveCount() == 1u);
}

/**
 * A polygon far larger than one grid is undiceable, so the second pass splits it and hands the
 * pieces back to the first, which measures each in turn. The recursion ends because a split
 * that does not shrink its input is not handed back at all - without that the pieces would be
 * re-split forever.
 **/
BOOST_AUTO_TEST_CASE(render_context_split_terminates_test) {
    v3d::moya::RenderContext rc;
    rc.prepareWorld();

    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.9f, -0.9f, 5.0f));
    polygon->addVertex(vertex(0.9f, -0.9f, 5.0f));
    polygon->addVertex(vertex(0.9f, 0.9f, 5.0f));
    polygon->addVertex(vertex(-0.9f, 0.9f, 5.0f));
    rc.addPolygon(polygon);

    rc.render();

    /*
        The default screen window is [-4/3, 4/3] by [-1, 1] over a 320 by 240 image, so the
        raster bound is 216 pixels across. A grid covers 16, so four rounds of four way
        splitting bring every piece under one: 4^4 pieces, all diceable and bucketed. Reaching
        the count at all is half the assertion - a split that failed to shrink its input would
        be re-split without end.
    */
    BOOST_TEST(rc.framebuffer()->primitiveCount() == 256u);
}

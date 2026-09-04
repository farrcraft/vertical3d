/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>

#include <boost/test/unit_test.hpp>

#include <glm/glm.hpp>

#include "../libmoya/RenderContext.h"

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

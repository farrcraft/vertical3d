/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/rib/Declaration.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/trace/Scene.h>
#include <api/render/offline/trace/Sphere.h>
#include <moya/libmoya/Planes.h>
#include <moya/libmoya/RIBHandler.h>
#include <moya/libmoya/Shading.h>
#include <spdlog/sinks/ringbuffer_sink.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Handling.h"

namespace v3d::moya::handling {
namespace {

/**
 * RiColor is graphics state rather than a material, and it reaches the pixels.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_color_test) {
    v3d::moya::RIBHandler handler;

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Color [1 0 0]\n"
        "Polygon \"P\" [-0.2 -0.2 5  0.2 -0.2 5  0.2 0.2 5  -0.2 0.2 5]\n"
        "WorldEnd\n", &handler));

    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().picture();
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::Planes::RED, 32, 24), 1.0F);
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::Planes::GREEN, 32, 24), 0.0F);
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::Planes::BLUE, 32, 24), 0.0F);
}

/**
 * CoordinateSystem names the current transformation, and CoordSysTransform makes it current
 * again after the transformation has moved on.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_coordinate_systems_test) {
    v3d::moya::RIBHandler handler;

    BOOST_REQUIRE(read(
        "Scale 2 2 2\n"
        "CoordinateSystem \"lamp\"\n"
        "Identity\n"
        "CoordSysTransform \"lamp\"\n"
        "CoordinateSystem \"copy\"\n", &handler));

    const glm::mat4x4 doubled = glm::scale(glm::mat4x4(1.0F), glm::vec3(2.0F));
    BOOST_TEST((handler.context().coordinateSystem("lamp") == doubled));
    BOOST_TEST((handler.context().coordinateSystem("copy") == doubled));
}

/**
 * A shader with an empty name names nothing, so the request is ignored and the surface in
 * force stays. The C interface passes RI_NULL on as the empty name.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_an_empty_name_is_ignored_test) {
    v3d::moya::RIBHandler handler;

    BOOST_REQUIRE(read(
        "Surface \"plastic\"\n"
        "Surface \"\"\n", &handler));

    const v3d::moya::Shading shading = handler.context().shading();
    BOOST_REQUIRE(shading.surface);
    BOOST_CHECK_EQUAL(shading.surface->name(), "plastic");
}

/**
 * The current colour and transform push and pop together, so the second polygon is not
 * coloured or placed by what the first one set.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_attribute_block_test) {
    v3d::moya::RIBHandler handler;

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "AttributeBegin\n"
        "Color [1 0 0]\n"
        "Translate -0.5 0 5\n"
        "Polygon \"P\" [-0.2 -0.2 0  0.2 -0.2 0  0.2 0.2 0  -0.2 0.2 0]\n"
        "AttributeEnd\n"
        "AttributeBegin\n"
        "Color [0 0 1]\n"
        "Translate 0.5 0 5\n"
        "Polygon \"P\" [-0.2 -0.2 0  0.2 -0.2 0  0.2 0.2 0  -0.2 0.2 0]\n"
        "AttributeEnd\n"
        "WorldEnd\n", &handler));

    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().picture();
    // the screen window is [-4/3, 4/3] by [-1, 1] on a 64 by 48 frame, so -0.5 in x lands at
    // column 20 and +0.5 at column 44
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::Planes::RED, 20, 24), 1.0F);
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::Planes::BLUE, 20, 24), 0.0F);
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::Planes::BLUE, 44, 24), 1.0F);
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::Planes::RED, 44, 24), 0.0F);
}

/**
 * TransformEnd restores the current transformation that TransformBegin saved.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_transform_block_test) {
    v3d::moya::RIBHandler handler;

    // a polygon added after the block sits where its own points say
    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "TransformBegin\n"
        "Translate 5 5 5\n"
        "TransformEnd\n"
        "Color [1 0 0]\n"
        "Polygon \"P\" [-0.2 -0.2 5  0.2 -0.2 5  0.2 0.2 5  -0.2 0.2 5]\n"
        "WorldEnd\n", &handler));

    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().picture();
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::Planes::RED, 32, 24), 1.0F);
}

/**
 * A primitive whose motion flattens it at both ends of the shutter has no pose to move it from.
 * A polygon under such a motion is left out of the frame, and a sphere out of the traced
 * scene. Neither throws nor leaves a pose that is not a number.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_motion_flat_at_both_ends_test) {
    const std::string flat =
        "Format 16 16 1\n"
        "Shutter 0 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "MotionBegin [0 1]\n"
        "Scale 0 1 1\n"
        "Scale 1 0 1\n"
        "MotionEnd\n";
    {
        v3d::moya::RIBHandler handler;
        BOOST_REQUIRE(read(flat + "Polygon \"P\" [-0.5 -0.5 2  0.5 -0.5 2  0.5 0.5 2  -0.5 0.5 2]\nWorldEnd\n",
            &handler));
        BOOST_CHECK_EQUAL(handler.context().bucketed(), 0U);
    }
    {
        v3d::moya::RIBHandler handler;
        BOOST_REQUIRE(read("Hider \"raytrace\"\n" + flat + "Sphere 1 -1 1 360\nWorldEnd\n", &handler));
        BOOST_CHECK(handler.context().traced().all<v3d::render::offline::trace::Sphere>().empty());
    }
}

/**
 * AreaLightSource in a file keeps the geometry after it in the same attribute block, polygons
 * and spheres alike, in world space. Geometry after the block, and an ordinary light, do not
 * join it.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_an_area_light_keeps_its_geometry_test) {
    v3d::moya::RIBHandler handler;

    // read up to WorldEnd, so the lights are still on to be read back
    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Hider \"raytrace\"\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "LightSource \"distantlight\" 1\n"
        "AttributeBegin\n"
        "AreaLightSource \"pointlight\" 2\n"
        "Translate 0 0 5\n"
        "Polygon \"P\" [0 0 0  2 0 0  2 1 0  0 1 0]\n"
        "Sphere 0.5 -0.5 0.5 360\n"
        "AttributeEnd\n"
        // which lights are on is an attribute, so the block's end switched the area light
        // off, and a scene switches it on again for what follows
        "Illuminate 2 1\n"
        "Polygon \"P\" [0 0 6  1 0 6  1 1 6  0 1 6]\n", &handler));

    const v3d::moya::Shading shading = handler.context().shading();
    BOOST_REQUIRE_EQUAL(shading.lights.size(), 2U);
    BOOST_CHECK(!shading.lights[0].area);
    BOOST_REQUIRE(shading.lights[1].area);
    // the quad's two units and the sphere's four pi a quarter
    const float pi = 3.14159265F;
    BOOST_CHECK_CLOSE(shading.lights[1].area->area(), 2.0F + pi, 0.01F);
    // a point on it is on the quad, five units out, or on the sphere around (0, 0, 5)
    const glm::vec3 point = shading.lights[1].area->sample(glm::vec2(0.1F, 0.5F));
    BOOST_CHECK_CLOSE(point.z, 5.0F, 0.01F);
    BOOST_CHECK(!handler.context().areaLight());
}

/**
 * Displacement in a file names the displacement shader, and Attribute "displacementbound"
 * bounds how far it moves a point. A bound is stated in object space unless it names another
 * space. Under a scale of three, a bound of 0.2 is 0.6 in camera space, and 0.2 when it is
 * stated in world space. A block's end restores both. An unrecognised attribute is reported and
 * changes nothing.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_a_displacement_reaches_the_context_test) {
    v3d::moya::RIBHandler handler;

    // read up to WorldEnd, so the attribute state is still there to be read back
    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Option \"searchpath\" \"shader\" [\"data:&\"]\n"
        "WorldBegin\n"
        "AttributeBegin\n"
        "Scale 3 3 3\n"
        "Attribute \"displacementbound\" \"float sphere\" [0.2]\n"
        "Displacement \"lift\" \"float amount\" [0.1]\n"
        "Attribute \"nothing\" \"float at\" [1]\n", &handler));

    v3d::moya::Shading shading = handler.context().shading();
    BOOST_REQUIRE(shading.displacement.shader);
    BOOST_CHECK_EQUAL(shading.displacement.shader->name(), "lift");
    BOOST_CHECK_CLOSE(shading.displacementBound, 0.6F, 0.01F);

    v3d::render::offline::rib::ParameterList inWorld;
    inWorld.add("sphere", v3d::render::offline::rib::Declaration(v3d::render::offline::rib::Declaration::Storage::UNIFORM,
        v3d::render::offline::rib::Declaration::Type::FLOAT, 1), { 0.2F }, std::vector<std::string>());
    inWorld.add("coordinatesystem", v3d::render::offline::rib::Declaration(
        v3d::render::offline::rib::Declaration::Storage::UNIFORM, v3d::render::offline::rib::Declaration::Type::STRING, 1),
        std::vector<float>(), { "world" });
    handler.context().attribute("displacementbound", inWorld);
    BOOST_CHECK_CLOSE(handler.context().shading().displacementBound, 0.2F, 0.01F);

    handler.context().attributeEnd();
    shading = handler.context().shading();
    BOOST_CHECK(!shading.displacement.shader);
    BOOST_CHECK_EQUAL(shading.displacementBound, 0.0F);
}

/**
 * A quad at z = 4 in its own space, moved to z = 5 by a translation after the surface, drawn
 * with a shader that shows the shading point in object space.
 **/
boost::shared_ptr<v3d::render::offline::FrameBuffer> inObjectSpace(const std::string & hider) {
    v3d::moya::RIBHandler handler;
    BOOST_REQUIRE(read(
        "Option \"searchpath\" \"shader\" [\"data:&\"]\n"
        "Format 8 6 1\n"
        "PixelSamples 1 1\n"
        "PixelFilter \"box\" 1 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n" + hider +
        "WorldBegin\n"
        "Surface \"objectspace\"\n"
        "Translate 0 0 1\n"
        "Polygon \"P\" [-5 -5 4  5 -5 4  5 5 4  -5 5 4]\n"
        "WorldEnd\n", &handler));
    return handler.context().picture();
}

/**
 * Both hiders answer "object" space with the primitive's own transformation, so the quad's
 * depth in its own space is 4 under either.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_object_space_under_both_hiders_test) {
    BOOST_CHECK_CLOSE(inObjectSpace("")->value(v3d::moya::Planes::BLUE, 4, 3), 4.0F, 0.01F);
    BOOST_CHECK_CLOSE(inObjectSpace("Hider \"raytrace\"\n")->value(v3d::moya::Planes::BLUE, 4, 3), 4.0F, 0.01F);
}

/**
 * A quad lit by one light, under the hider given, with the surface and light named.
 **/
boost::shared_ptr<v3d::render::offline::FrameBuffer> messaged(const std::string & hider, const std::string & light,
    const std::string & surface) {
    v3d::moya::RIBHandler handler;
    BOOST_REQUIRE(read(
        "Option \"searchpath\" \"shader\" [\"data:&\"]\n"
        "Format 8 6 1\n"
        "PixelSamples 1 1\n"
        "PixelFilter \"box\" 1 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n" + hider +
        "WorldBegin\n"
        "LightSource \"" + light + "\" 1\n"
        "Surface \"" + surface + "\"\n"
        "Polygon \"P\" [-5 -5 5  -5 5 5  5 5 5  5 -5 5]\n"
        "WorldEnd\n", &handler));
    return handler.context().picture();
}

/**
 * Lights and surfaces pass messages under both hiders, as a traced hit runs its lights and its
 * surface as a grid does.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_messages_under_both_hiders_test) {
    for (const std::string & hider : { std::string(), std::string("Hider \"raytrace\"\n") }) {
        BOOST_TEST_CONTEXT("hider '" << hider << "'") {
            BOOST_CHECK_CLOSE(messaged(hider, "messenger", "heard")->value(v3d::moya::Planes::RED, 4, 3), 0.5F, 0.01F);
            BOOST_CHECK_CLOSE(messaged(hider, "asker", "told")->value(v3d::moya::Planes::RED, 4, 3), 0.75F, 0.01F);
        }
    }
}

};  // namespace
};  // namespace v3d::moya::handling

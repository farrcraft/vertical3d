/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Compare.h>
#include <api/image/Factory.h>
#include <api/render/offline/rib/Reader.h>
#include <moya/libmoya/RIBHandler.h>
#include <moya/libmoya/RenderContext.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/operations.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {

const char* REFERENCE = "data/reference-polygon.png";
const char* RENDERED = "data_out/reference-polygon.png";
const char* RIB_SCENE = "data/reference-polygon.rib";
const char* RIB_RENDERED = "data_out/reference-polygon-rib.png";

const char* SHADED = "data/reference-shaded.png";
const char* SHADED_RENDERED = "data_out/reference-shaded.png";
const char* SHADED_SCENE = "data/reference-shaded.rib";
const char* SHADED_RIB_RENDERED = "data_out/reference-shaded-rib.png";

const char* SAMPLED = "data/reference-sampled.png";
const char* SAMPLED_RENDERED = "data_out/reference-sampled.png";
const char* SAMPLED_SCENE = "data/reference-sampled.rib";
const char* SAMPLED_RIB_RENDERED = "data_out/reference-sampled-rib.png";
const char* SHADOW = "data/reference-shadow.png";
const char* SHADOW_SCENE = "data/reference-shadow.rib";
const char* SHADOW_RIB_RENDERED = "data_out/reference-shadow-rib.png";

const char* FOCUS = "data/reference-focus.png";
const char* FOCUS_RENDERED = "data_out/reference-focus.png";
const char* FOCUS_SCENE = "data/reference-focus.rib";
const char* FOCUS_RIB_RENDERED = "data_out/reference-focus-rib.png";

const char* MOTION = "data/reference-motion.png";
const char* MOTION_RENDERED = "data_out/reference-motion.png";
const char* MOTION_SCENE = "data/reference-motion.rib";
const char* MOTION_RIB_RENDERED = "data_out/reference-motion-rib.png";

const char* TRIANGLE = "data/raytrace-triangle.png";
const char* TRIANGLE_RENDERED = "data_out/raytrace-triangle.png";
const char* TRIANGLE_SCENE = "data/raytrace-triangle.rib";
const char* TRIANGLE_RIB_RENDERED = "data_out/raytrace-triangle-rib.png";

const char* TRACED_SHADED = "data/raytrace-shaded.png";
const char* TRACED_SHADED_RENDERED = "data_out/raytrace-shaded.png";
const char* TRACED_SHADED_SCENE = "data/raytrace-shaded.rib";
const char* TRACED_SHADED_RIB_RENDERED = "data_out/raytrace-shaded-rib.png";

const char* TRACED_SAMPLED = "data/raytrace-sampled.png";
const char* TRACED_SAMPLED_RENDERED = "data_out/raytrace-sampled.png";
const char* TRACED_SAMPLED_SCENE = "data/raytrace-sampled.rib";
const char* TRACED_SAMPLED_RIB_RENDERED = "data_out/raytrace-sampled-rib.png";

const char* TRACED_FOCUS = "data/raytrace-focus.png";
const char* TRACED_FOCUS_RENDERED = "data_out/raytrace-focus.png";
const char* TRACED_FOCUS_SCENE = "data/raytrace-focus.rib";
const char* TRACED_FOCUS_RIB_RENDERED = "data_out/raytrace-focus-rib.png";

const char* TRACED_MOTION = "data/raytrace-motion.png";
const char* TRACED_MOTION_RENDERED = "data_out/raytrace-motion.png";
const char* TRACED_MOTION_SCENE = "data/raytrace-motion.rib";
const char* TRACED_MOTION_RIB_RENDERED = "data_out/raytrace-motion-rib.png";

const char* SPHERES = "data/raytrace-spheres.png";
const char* SPHERES_RENDERED = "data_out/raytrace-spheres.png";
const char* SPHERES_SCENE = "data/raytrace-spheres.rib";
const char* SPHERES_RIB_RENDERED = "data_out/raytrace-spheres-rib.png";

/** The two hiders, for a property both are expected to have. **/
const char* HIDERS[] = { "hidden", "raytrace" };

/**
 * One sample at each pixel centre under a one pixel box, which the film gives back exactly.
 * The references drawn this way pin the hider and the shading; reference-sampled pins the
 * sampling.
 **/
void pixelCentres(v3d::moya::RenderContext & rc) {
    rc.sampling().samples = glm::uvec2(1, 1);
    rc.sampling().filter = v3d::render::offline::Filter::Box;
    rc.sampling().width = glm::vec2(1.0f, 1.0f);
}

v3d::moya::Vertex vertex(float x, float y, float z) {
    v3d::moya::Vertex v;
    v.point(glm::vec3(x, y, z));
    return v;
}

/*
    A quad facing the camera, asymmetric about both axes so that a flipped picture is a
    failing one. The screen window is [-1, 1] on both axes, and the raster transform puts
    its origin at the upper left corner.

    Regenerating this reference is expected whenever shading or sampling changes the
    picture on purpose. What it buys is that a change which was not meant to alter the
    picture says so.
*/
void scene(v3d::moya::RenderContext & rc) {
    rc.imageResolution(64, 48, 1.0f);
    pixelCentres(rc);
    // the defaults are RI_EPSILON and RI_INFINITY, which leave the orthographic depth
    // scale at about 2e-38 and collapse every z onto the near plane
    rc.clipping(1.0f, 100.0f);
    rc.prepareWorld();
    // the shader that means no shading, built rather than defaulted to, so that this
    // picture goes through the language by both routes
    rc.surface("constant", v3d::render::offline::rib::ParameterList());

    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.7f, -0.2f, 5.0f));
    polygon->addVertex(vertex(0.3f, -0.2f, 5.0f));
    polygon->addVertex(vertex(0.3f, 0.8f, 5.0f));
    polygon->addVertex(vertex(-0.7f, 0.8f, 5.0f));
    rc.addPolygon(polygon);
}

/**
 * A vertex with a shading normal of its own, which is what makes a quad's cosine falloff a
 * gradient rather than one value.
 **/
v3d::moya::Vertex shaded(float x, float y, float z, const glm::vec3 & normal) {
    v3d::moya::Vertex v;
    v.point(glm::vec3(x, y, z));
    v.normal(normal);
    return v;
}

void quad(v3d::moya::RenderContext & rc, float left, float right,
    const glm::vec3 & outer, const glm::vec3 & inner) {
    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(shaded(left, -0.8f, 5.0f, outer));
    polygon->addVertex(shaded(right, -0.8f, 5.0f, inner));
    polygon->addVertex(shaded(right, 0.8f, 5.0f, inner));
    polygon->addVertex(shaded(left, 0.8f, 5.0f, outer));
    rc.addPolygon(polygon);
}

/**
 * One parameter as a scene wrote it, typed the way the reader would have typed it.
 *
 * An out parameter rather than an answer, which is what rib::arguments does beside it:
 * a ParameterList is a map and returning one by value is a copy of every parameter a
 * request carried.
 **/
void put(v3d::render::offline::rib::ParameterList* list, const std::string & name,
    v3d::render::offline::rib::Declaration::Type type, const std::vector<float> & values) {
    list->add(name, v3d::render::offline::rib::Declaration(
        v3d::render::offline::rib::Declaration::Storage::UNIFORM, type, 1), values,
        std::vector<std::string>());
}

/*
    The same scene as data/reference-shaded.rib, built through the render context instead
    of read from a file. Two surfaces under two lights: a matte one and a plastic one, a
    distant light and a point light close enough for its falloff to show. It is sampled at
    the RI defaults unless the caller says otherwise.
*/
void shadedScene(v3d::moya::RenderContext & rc) {
    rc.imageResolution(64, 48, 1.0f);
    rc.clipping(1.0f, 100.0f);
    rc.prepareWorld();

    typedef v3d::render::offline::rib::Declaration Declaration;
    v3d::render::offline::rib::ParameterList distant;
    put(&distant, "intensity", Declaration::Type::FLOAT, { 0.8f });
    put(&distant, "to", Declaration::Type::POINT, { 0.6f, -0.4f, 1.0f });
    rc.lightSource("distantlight", "1", distant);

    v3d::render::offline::rib::ParameterList bulb;
    put(&bulb, "intensity", Declaration::Type::FLOAT, { 4.0f });
    rc.pushTransform();
    rc.translate(-1.0f, 1.0f, 2.0f);
    rc.lightSource("pointlight", "2", bulb);
    rc.popTransform();

    const glm::vec3 tilted(-0.9f, 0.0f, -0.436f);
    const glm::vec3 square(0.0f, 0.0f, -1.0f);

    rc.attributeBegin();
    rc.color(glm::vec3(0.9f, 0.4f, 0.2f));
    rc.surface("matte", v3d::render::offline::rib::ParameterList());
    quad(rc, -1.2f, -0.1f, tilted, square);
    rc.attributeEnd();

    rc.attributeBegin();
    rc.color(glm::vec3(0.3f, 0.5f, 0.9f));
    v3d::render::offline::rib::ParameterList plastic;
    put(&plastic, "roughness", Declaration::Type::FLOAT, { 0.08f });
    put(&plastic, "Ks", Declaration::Type::FLOAT, { 0.6f });
    rc.surface("plastic", plastic);
    quad(rc, 0.1f, 1.2f, square, glm::vec3(0.9f, 0.0f, -0.436f));
    rc.attributeEnd();
}

/*
    The same scene as data/reference-focus.rib: a red quad on the plane of focus four units
    out and a blue one ten units out, through a perspective camera. The lens is the file's,
    or a pinhole.
*/
void focusScene(v3d::moya::RenderContext & rc, bool lens) {
    rc.imageResolution(64, 48, 1.0f);
    rc.sampling().samples = glm::uvec2(4, 4);
    if (lens) {
        rc.sampling().fstop = 1.0f;
        rc.sampling().focalLength = 0.5f;
        rc.sampling().focalDistance = 4.0f;
    }
    rc.projection("perspective", 40.0f);
    rc.clipping(0.1f, 100.0f);
    rc.prepareWorld();
    rc.surface("constant", v3d::render::offline::rib::ParameterList());

    rc.color(glm::vec3(0.9f, 0.2f, 0.2f));
    boost::shared_ptr<v3d::moya::Polygon> near = boost::make_shared<v3d::moya::Polygon>();
    near->addVertex(vertex(-1.5f, -1.0f, 4.0f));
    near->addVertex(vertex(-0.2f, -1.0f, 4.0f));
    near->addVertex(vertex(-0.2f, 1.0f, 4.0f));
    near->addVertex(vertex(-1.5f, 1.0f, 4.0f));
    rc.addPolygon(near);

    rc.color(glm::vec3(0.2f, 0.4f, 0.9f));
    boost::shared_ptr<v3d::moya::Polygon> far = boost::make_shared<v3d::moya::Polygon>();
    far->addVertex(vertex(0.5f, -2.5f, 10.0f));
    far->addVertex(vertex(4.0f, -2.5f, 10.0f));
    far->addVertex(vertex(4.0f, 2.5f, 10.0f));
    far->addVertex(vertex(0.5f, 2.5f, 10.0f));
    rc.addPolygon(far);
}

/**
 * How many pixels along a row are partly covered, which is how wide an edge is.
 **/
unsigned int partial(const v3d::render::offline::FrameBuffer & planes, unsigned int coverage,
    unsigned int row, unsigned int from, unsigned int to) {
    unsigned int count = 0;
    for (unsigned int column = from; column <= to; column++) {
        const float value = planes.value(coverage, column, row);
        if (value > 0.02f && value < 0.98f) {
            count++;
        }
    }
    return count;
}

/**
 * The largest difference between two frames over every plane of a block of columns.
 **/
float largest(const v3d::render::offline::FrameBuffer & a, const v3d::render::offline::FrameBuffer & b,
    unsigned int from, unsigned int to) {
    float worst = 0.0f;
    for (unsigned int plane = 0; plane < a.planes(); plane++) {
        for (unsigned int row = 0; row < a.height(); row++) {
            for (unsigned int column = from; column <= to; column++) {
                worst = std::max(worst, std::fabs(a.value(plane, column, row) - b.value(plane, column, row)));
            }
        }
    }
    return worst;
}

/**
 * The orthographic camera of the other references, with the shutter open from 0 to 1.
 **/
void motionCamera(v3d::moya::RenderContext & rc) {
    rc.imageResolution(64, 48, 1.0f);
    rc.sampling().samples = glm::uvec2(4, 4);
    rc.sampling().shutter = glm::vec2(0.0f, 1.0f);
    rc.clipping(0.1f, 100.0f);
    rc.prepareWorld();
    rc.surface("constant", v3d::render::offline::rib::ParameterList());
}

void quadAt(v3d::moya::RenderContext & rc, const glm::vec3 (&corners)[4]) {
    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    for (const glm::vec3 & corner : corners) {
        polygon->addVertex(vertex(corner.x, corner.y, corner.z));
    }
    rc.addPolygon(polygon);
}

/*
    The same scene as data/reference-motion.rib: a quad sliding right and a quad turning a
    quarter about its centre, each across the whole shutter.
*/
void motionScene(v3d::moya::RenderContext & rc) {
    motionCamera(rc);

    rc.attributeBegin();
    rc.motionBegin({ 0.0f, 1.0f });
    rc.translate(0.0f, 0.0f, 0.0f);
    rc.translate(0.5f, 0.0f, 0.0f);
    rc.motionEnd();
    rc.color(glm::vec3(0.9f, 0.8f, 0.2f));
    const glm::vec3 slid[4] = {
        glm::vec3(-1.2f, 0.1f, 5.0f), glm::vec3(-0.4f, 0.1f, 5.0f),
        glm::vec3(-0.4f, 0.8f, 5.0f), glm::vec3(-1.2f, 0.8f, 5.0f)
    };
    quadAt(rc, slid);
    rc.attributeEnd();

    rc.attributeBegin();
    rc.translate(0.6f, -0.4f, 5.0f);
    rc.motionBegin({ 0.0f, 1.0f });
    rc.rotate(0.0f, 0.0f, 0.0f, 1.0f);
    rc.rotate(90.0f, 0.0f, 0.0f, 1.0f);
    rc.motionEnd();
    rc.color(glm::vec3(0.2f, 0.7f, 0.9f));
    const glm::vec3 turned[4] = {
        glm::vec3(-0.35f, -0.15f, 0.0f), glm::vec3(0.35f, -0.15f, 0.0f),
        glm::vec3(0.35f, 0.15f, 0.0f), glm::vec3(-0.35f, 0.15f, 0.0f)
    };
    quadAt(rc, turned);
    rc.attributeEnd();
}

/**
 * The coverage of a quad that slid twelve pixels right while the shutter was open, along the
 * row through its middle. Its left edge leaves pixels 8 to 19 one after another and its right
 * edge reaches pixels 32 to 43, so under a one pixel box the coverage climbs a twelfth a pixel
 * from 8 and falls a twelfth a pixel from 32, and is whole between.
 **/
void checkRamp(const v3d::render::offline::FrameBuffer & planes, unsigned int coverage, unsigned int row) {
    BOOST_CHECK_EQUAL(planes.value(coverage, 7, row), 0.0f);
    for (unsigned int k = 0; k < 12; k++) {
        BOOST_TEST_CONTEXT("pixel " << 8 + k << " and " << 32 + k) {
            const float rising = (static_cast<float>(k) + 0.5f) / 12.0f;
            BOOST_CHECK_SMALL(planes.value(coverage, 8 + k, row) - rising, 0.1f);
            BOOST_CHECK_SMALL(planes.value(coverage, 32 + k, row) - (1.0f - rising), 0.1f);
        }
    }
    BOOST_CHECK_CLOSE(planes.value(coverage, 25, row), 1.0f, 1.0e-4f);
    BOOST_CHECK_EQUAL(planes.value(coverage, 44, row), 0.0f);
}

/**
 * A rendered image against a committed one, writing what it rendered when they differ.
 **/
void check(const boost::shared_ptr<v3d::image::Image> & rendered, const char* reference,
    const char* output) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);
    boost::shared_ptr<v3d::image::Image> committed = factory.read(reference);
    if (committed == nullptr) {
        boost::filesystem::create_directory("data_out");
        factory.write(output, rendered);
    }
    BOOST_REQUIRE_MESSAGE(committed != nullptr,
        std::string("cannot read the reference image ") + reference +
        " - what was rendered is in " + output);

    v3d::image::Difference difference = v3d::image::compare(*rendered, *committed, 1);
    if (!difference.match) {
        boost::filesystem::create_directory("data_out");
        factory.write(output, rendered);
    }
    BOOST_CHECK_MESSAGE(difference.match,
        difference.description() + " - what was rendered instead is in " + output);
}

/**
 * A frame read from a file, under the hider the file names unless the caller names one first.
 **/
boost::shared_ptr<v3d::render::offline::FrameBuffer> read(const char* scene, const char* hider = nullptr) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    if (hider != nullptr) {
        handler.context().hider(hider);
    }
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());
    BOOST_REQUIRE(reader.read(scene, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");
    return handler.context().framebuffer()->planes();
}

boost::shared_ptr<v3d::image::Image> picture(const v3d::moya::RenderContext & rc) {
    return rc.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS);
}

/**
 * The camera the ray traced references share: orthographic over the default 4:3 screen
 * window, with the world origin one unit in front of the eye. The world is not begun, so a
 * caller can still name an imager.
 **/
void raytraceCamera(v3d::moya::RenderContext & rc) {
    rc.hider("raytrace");
    rc.imageResolution(64, 48, 1.0f);
    rc.clipping(0.001f, 100.0f);
    rc.projection("orthographic");
    rc.setTransform(glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
}

void background(v3d::moya::RenderContext & rc, const glm::vec3 & colour) {
    v3d::render::offline::rib::ParameterList list;
    put(&list, "background", v3d::render::offline::rib::Declaration::Type::COLOR, { colour.r, colour.g, colour.b });
    rc.imager("background", list);
}

/*
    The same scene as data/raytrace-triangle.rib: a triangle asymmetric about both axes, so a
    flipped picture is a failing one, drawn by the shader that means no shading.
*/
void triangleScene(v3d::moya::RenderContext & rc) {
    raytraceCamera(rc);
    pixelCentres(rc);
    background(rc, glm::vec3(0.15f, 0.25f, 0.45f));
    rc.prepareWorld();
    rc.surface("constant", v3d::render::offline::rib::ParameterList());
    rc.color(glm::vec3(0.9f, 0.2f, 0.2f));
    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.8f, -0.6f, 2.0f));
    polygon->addVertex(vertex(0.8f, -0.6f, 2.0f));
    polygon->addVertex(vertex(0.0f, 0.7f, 2.0f));
    rc.addPolygon(polygon);
}

/*
    The same scene as data/raytrace-shaded.rib: a matte floor and a plastic panel, three
    lights of three kinds, and a shadow the panel casts across the floor. It is sampled at the
    RI defaults unless the caller says otherwise.
*/
void raytraceShadedScene(v3d::moya::RenderContext & rc) {
    typedef v3d::render::offline::rib::Declaration Declaration;
    raytraceCamera(rc);
    background(rc, glm::vec3(0.05f, 0.06f, 0.1f));
    rc.prepareWorld();

    v3d::render::offline::rib::ParameterList fill;
    put(&fill, "intensity", Declaration::Type::FLOAT, { 0.18f });
    rc.lightSource("ambientlight", "0", fill);
    v3d::render::offline::rib::ParameterList distant;
    put(&distant, "intensity", Declaration::Type::FLOAT, { 0.9f });
    put(&distant, "to", Declaration::Type::POINT, { 0.7f, -0.7f, 1.0f });
    rc.lightSource("distantlight", "1", distant);
    v3d::render::offline::rib::ParameterList lamp;
    put(&lamp, "intensity", Declaration::Type::FLOAT, { 1.2f });
    rc.pushTransform();
    rc.translate(-0.6f, 0.5f, 1.2f);
    rc.lightSource("pointlight", "2", lamp);
    rc.popTransform();

    rc.attributeBegin();
    rc.color(glm::vec3(0.8f, 0.75f, 0.6f));
    rc.surface("matte", v3d::render::offline::rib::ParameterList());
    const glm::vec3 floor[4] = {
        glm::vec3(-1.15f, -1.2f, 3.0f), glm::vec3(1.15f, -1.2f, 3.0f),
        glm::vec3(1.15f, 1.2f, 3.0f), glm::vec3(-1.15f, 1.2f, 3.0f)
    };
    quadAt(rc, floor);
    rc.attributeEnd();

    rc.attributeBegin();
    rc.color(glm::vec3(0.2f, 0.45f, 0.8f));
    v3d::render::offline::rib::ParameterList shiny;
    put(&shiny, "roughness", Declaration::Type::FLOAT, { 0.1f });
    put(&shiny, "Ks", Declaration::Type::FLOAT, { 0.5f });
    rc.surface("plastic", shiny);
    const glm::vec3 panel[4] = {
        glm::vec3(-0.5f, -0.45f, 2.2f), glm::vec3(0.35f, -0.45f, 2.2f),
        glm::vec3(0.35f, 0.4f, 2.2f), glm::vec3(-0.5f, 0.4f, 2.2f)
    };
    quadAt(rc, panel);
    rc.attributeEnd();
}

/*
    The same scene as data/raytrace-spheres.rib: a metal sphere and a glass one over a checked
    floor, in front of a wall, through a perspective camera at the origin and sampled at the RI
    defaults.
*/
void spheresScene(v3d::moya::RenderContext & rc) {
    typedef v3d::render::offline::rib::Declaration Declaration;
    rc.hider("raytrace");
    rc.imageResolution(64, 48, 1.0f);
    rc.clipping(0.1f, 100.0f);
    rc.projection("perspective", 40.0f);
    // the floor's shader is a fixture beside the scene rather than one of the standard ones
    rc.searchpath("data:&");
    rc.prepareWorld();

    v3d::render::offline::rib::ParameterList fill;
    put(&fill, "intensity", Declaration::Type::FLOAT, { 0.2f });
    rc.lightSource("ambientlight", "0", fill);
    v3d::render::offline::rib::ParameterList distant;
    put(&distant, "intensity", Declaration::Type::FLOAT, { 0.9f });
    put(&distant, "to", Declaration::Type::POINT, { 0.4f, -1.0f, 0.6f });
    rc.lightSource("distantlight", "1", distant);

    rc.attributeBegin();
    rc.color(glm::vec3(0.85f, 0.8f, 0.7f));
    v3d::render::offline::rib::ParameterList checks;
    put(&checks, "size", Declaration::Type::FLOAT, { 0.5f });
    rc.surface("checked", checks);
    const glm::vec3 floor[4] = {
        glm::vec3(-4.0f, -1.0f, 2.0f), glm::vec3(4.0f, -1.0f, 2.0f),
        glm::vec3(4.0f, -1.0f, 12.0f), glm::vec3(-4.0f, -1.0f, 12.0f)
    };
    quadAt(rc, floor);
    rc.attributeEnd();

    rc.attributeBegin();
    rc.color(glm::vec3(0.35f, 0.5f, 0.75f));
    rc.surface("matte", v3d::render::offline::rib::ParameterList());
    const glm::vec3 wall[4] = {
        glm::vec3(-4.0f, -1.0f, 12.0f), glm::vec3(4.0f, -1.0f, 12.0f),
        glm::vec3(4.0f, 4.0f, 12.0f), glm::vec3(-4.0f, 4.0f, 12.0f)
    };
    quadAt(rc, wall);
    rc.attributeEnd();

    rc.attributeBegin();
    rc.color(glm::vec3(0.9f, 0.85f, 0.7f));
    v3d::render::offline::rib::ParameterList mirror;
    put(&mirror, "Ka", Declaration::Type::FLOAT, { 0.1f });
    put(&mirror, "Ks", Declaration::Type::FLOAT, { 0.6f });
    put(&mirror, "Kr", Declaration::Type::FLOAT, { 0.8f });
    rc.surface("shinymetal", mirror);
    rc.translate(-1.1f, -0.28f, 6.0f);
    BOOST_REQUIRE(rc.addSphere(0.7f, -0.7f, 0.7f, 360.0f));
    rc.attributeEnd();

    rc.attributeBegin();
    rc.color(glm::vec3(1.0f));
    rc.opacity(glm::vec3(0.3f));
    rc.surface("glass", v3d::render::offline::rib::ParameterList());
    rc.translate(1.1f, -0.28f, 5.5f);
    BOOST_REQUIRE(rc.addSphere(0.7f, -0.7f, 0.7f, 360.0f));
    rc.attributeEnd();
}

/**
 * A quad sliding half a unit right across the whole shutter, sampled finely under a one pixel
 * box, which is twelve pixels at this frame.
 **/
void slidingQuad(v3d::moya::RenderContext & rc) {
    rc.imageResolution(64, 48, 1.0f);
    rc.sampling().samples = glm::uvec2(8, 8);
    rc.sampling().filter = v3d::render::offline::Filter::Box;
    rc.sampling().width = glm::vec2(1.0f);
    rc.sampling().shutter = glm::vec2(0.0f, 1.0f);
    rc.clipping(0.1f, 100.0f);
    rc.prepareWorld();
    rc.surface("constant", v3d::render::offline::rib::ParameterList());

    rc.motionBegin({ 0.0f, 1.0f });
    rc.translate(0.0f, 0.0f, 0.0f);
    rc.translate(0.5f, 0.0f, 0.0f);
    rc.motionEnd();
    const glm::vec3 corners[4] = {
        glm::vec3(-1.0f, -0.5f, 5.0f), glm::vec3(0.0f, -0.5f, 5.0f),
        glm::vec3(0.0f, 0.5f, 5.0f), glm::vec3(-1.0f, 0.5f, 5.0f)
    };
    quadAt(rc, corners);
}

};  // namespace

/**
 * The second pass writes samples where the polygon is and leaves the rest of the planes at
 * what they were allocated with.
 **/
BOOST_AUTO_TEST_CASE(moya_renders_a_polygon_test) {
    v3d::moya::RenderContext rc;
    scene(rc);
    rc.render();

    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = rc.framebuffer()->planes();

    // the quad covers raster x over [15.2, 39.2] and y over [4.8, 28.8]
    BOOST_TEST(planes->value(v3d::moya::FrameBuffer::RED, 24, 16) == 1.0f);
    BOOST_TEST(planes->value(v3d::moya::FrameBuffer::BLUE, 24, 16) == 1.0f);
    // and nothing outside it
    BOOST_TEST(planes->value(v3d::moya::FrameBuffer::RED, 55, 40) == 0.0f);

    // a covered pixel took a depth from the geometry rather than keeping the one it was
    // cleared to
    BOOST_TEST(planes->value(v3d::moya::FrameBuffer::DEPTH, 24, 16) < 2.0f);
}

/**
 * A rendered image against a committed one. moya touches no window, device or
 * swapchain, so unlike everything below the recorder in api/render this runs in CI.
 **/
BOOST_AUTO_TEST_CASE(moya_reference_test) {
    v3d::moya::RenderContext rc;
    scene(rc);
    rc.render();

    boost::shared_ptr<v3d::image::Image> rendered =
        rc.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS);

    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    boost::shared_ptr<v3d::image::Image> reference = factory.read(REFERENCE);
    if (reference == nullptr) {
        // chasing a failure without the image that caused it is most of the cost of a
        // reference test, and a reference being regenerated on purpose starts here too
        boost::filesystem::create_directory("data_out");
        factory.write(RENDERED, rendered);
    }
    BOOST_REQUIRE_MESSAGE(reference != nullptr,
        std::string("cannot read the reference image ") + REFERENCE + " - what was rendered is in " + RENDERED);

    // the tolerance is for float rounding across compilers, not for "close enough"
    v3d::image::Difference difference = v3d::image::compare(*rendered, *reference, 1);
    if (!difference.match) {
        boost::filesystem::create_directory("data_out");
        factory.write(RENDERED, rendered);
    }
    BOOST_CHECK_MESSAGE(difference.match,
        difference.description() + " - what was rendered instead is in " + RENDERED);
}

/**
 * The same scene said in RIB, against the same committed picture.
 *
 * Two routes to one image: if the file path and the code path disagree, this says so, and
 * neither of them is the reference. It is what makes the reader a rendering change rather
 * than a parsing one.
 **/
BOOST_AUTO_TEST_CASE(moya_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(RIB_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    boost::shared_ptr<v3d::image::Image> rendered =
        handler.context().framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS);

    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);
    boost::shared_ptr<v3d::image::Image> reference = factory.read(REFERENCE);
    BOOST_REQUIRE(reference != nullptr);

    v3d::image::Difference difference = v3d::image::compare(*rendered, *reference, 1);
    if (!difference.match) {
        boost::filesystem::create_directory("data_out");
        factory.write(RIB_RENDERED, rendered);
    }
    BOOST_CHECK_MESSAGE(difference.match,
        difference.description() + " - what the rib scene rendered instead is in " + RIB_RENDERED);
}

/**
 * A display of type "file" is what turns the sampled planes into a picture on disk; the
 * driver's --output reaches the context through RiDisplay and nothing else writes.
 **/
BOOST_AUTO_TEST_CASE(moya_display_writes_a_file_test) {
    boost::filesystem::create_directory("data_out");
    const char* output = "data_out/display.png";
    boost::filesystem::remove(output);

    v3d::moya::RenderContext rc;
    scene(rc);
    rc.display(output, "file", "rgb");
    rc.render();

    BOOST_TEST(boost::filesystem::exists(output));
}

/**
 * A context with no display leaves its samples in the planes.
 **/
BOOST_AUTO_TEST_CASE(moya_no_display_writes_nothing_test) {
    v3d::moya::RenderContext rc;
    scene(rc);
    rc.render();

    BOOST_TEST(rc.framebuffer()->planes()->value(v3d::moya::FrameBuffer::RED, 24, 16) == 1.0f);
}

/**
 * The first moya picture with shading in it: a matte surface and a plastic one, a distant
 * light and a point light, each surface carrying normals of its own so the falloff is a
 * gradient.
 *
 * A shader is not tested by a picture - the language's own cases are in
 * v3dtest_render_offline, and a wrong smoothstep is found there. What this catches is the
 * wiring between the machine and the renderer, which no unit case can see.
 **/
BOOST_AUTO_TEST_CASE(moya_shaded_reference_test) {
    v3d::moya::RenderContext rc;
    shadedScene(rc);
    pixelCentres(rc);
    rc.render();

    check(rc.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS),
        SHADED, SHADED_RENDERED);
}

/**
 * The same shaded scene said in RIB, against the same committed picture.
 *
 * Two routes to one image, which is what phase 2 established and what a scene with a
 * shader in it has more of to disagree about: a parameter bound on one path and defaulted
 * on the other would show here and nowhere else.
 **/
BOOST_AUTO_TEST_CASE(moya_shaded_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(SHADED_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS),
        SHADED, SHADED_RIB_RENDERED);
}

/**
 * The shaded scene at the RI defaults, two by two samples under a gaussian two pixels wide,
 * per ADR-0076. Its edges are antialiased, and it is the same on every run because every
 * pixel's samples are seeded by where the pixel is.
 **/
BOOST_AUTO_TEST_CASE(moya_sampled_reference_test) {
    v3d::moya::RenderContext rc;
    shadedScene(rc);
    rc.render();

    check(rc.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS),
        SAMPLED, SAMPLED_RENDERED);
}

/**
 * The sampled scene said in RIB, which names no sampling and so gets the defaults.
 **/
BOOST_AUTO_TEST_CASE(moya_sampled_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(SAMPLED_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS),
        SAMPLED, SAMPLED_RIB_RENDERED);
}

/**
 * Two renders of the sampled scene are equal byte for byte.
 **/
BOOST_AUTO_TEST_CASE(moya_sampled_render_is_repeatable_test) {
    v3d::moya::RenderContext first;
    shadedScene(first);
    first.render();
    v3d::moya::RenderContext second;
    shadedScene(second);
    second.render();

    const v3d::image::Difference difference = v3d::image::compare(
        *first.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS),
        *second.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS), 0);
    BOOST_CHECK_MESSAGE(difference.match, difference.description());
}

/**
 * A primitive straddling a bucket edge renders as it does inside one bucket. The samples
 * belong to the frame rather than to a bucket, and the film filters across bucket edges, so
 * where the buckets fall does not show.
 **/
BOOST_AUTO_TEST_CASE(moya_bucket_edges_do_not_show_test) {
    v3d::moya::RenderContext small;
    small.bucketSize(8, 8);
    shadedScene(small);
    small.render();
    v3d::moya::RenderContext large;
    large.bucketSize(64, 64);
    shadedScene(large);
    large.render();

    const v3d::image::Difference difference = v3d::image::compare(
        *small.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS),
        *large.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS), 0);
    BOOST_CHECK_MESSAGE(difference.match, difference.description());
}

/**
 * Two quads at two depths through a lens focused on the nearer one.
 **/
BOOST_AUTO_TEST_CASE(moya_focus_reference_test) {
    v3d::moya::RenderContext rc;
    focusScene(rc, true);
    rc.render();

    check(rc.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS), FOCUS, FOCUS_RENDERED);
}

BOOST_AUTO_TEST_CASE(moya_focus_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(FOCUS_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS),
        FOCUS, FOCUS_RIB_RENDERED);
}

/**
 * The quad on the plane of focus is as sharp through the lens as through a pinhole: a point
 * at the focal distance moves by nothing whatever the lens point. The block is the columns
 * the blue quad's blur does not reach.
 **/
BOOST_AUTO_TEST_CASE(moya_in_focus_is_sharp_test) {
    for (const char* hider : HIDERS) {
        BOOST_TEST_CONTEXT("hider " << hider) {
            v3d::moya::RenderContext pinhole;
            pinhole.hider(hider);
            focusScene(pinhole, false);
            pinhole.render();
            v3d::moya::RenderContext lens;
            lens.hider(hider);
            focusScene(lens, true);
            lens.render();

            BOOST_CHECK_SMALL(largest(*pinhole.framebuffer()->planes(), *lens.framebuffer()->planes(), 0, 30),
                1.0f / 255.0f);
        }
    }
}

/**
 * The quad off the plane of focus spreads its edge over its circle of confusion. Under a one
 * pixel box, a pinhole leaves the edge in a pixel or two; the lens, a quarter of a unit across
 * and focused four units out, blurs a point ten units out over 2 * 0.25 * (10 - 4) / 10 of a
 * unit there, which is about five pixels at this field of view. The reyes hider gets there by
 * moving the micropolygon and the ray hider by moving the ray.
 **/
BOOST_AUTO_TEST_CASE(moya_out_of_focus_spreads_test) {
    for (const char* hider : HIDERS) {
        BOOST_TEST_CONTEXT("hider " << hider) {
            v3d::moya::RenderContext pinhole;
            pinhole.hider(hider);
            focusScene(pinhole, false);
            pinhole.sampling().filter = v3d::render::offline::Filter::Box;
            pinhole.sampling().width = glm::vec2(1.0f);
            pinhole.render();
            v3d::moya::RenderContext lens;
            lens.hider(hider);
            focusScene(lens, true);
            lens.sampling().filter = v3d::render::offline::Filter::Box;
            lens.sampling().width = glm::vec2(1.0f);
            lens.render();

            const unsigned int coverage = v3d::moya::FrameBuffer::COVERAGE;
            const unsigned int sharp = partial(*pinhole.framebuffer()->planes(), coverage, 24, 30, 45);
            const unsigned int blurred = partial(*lens.framebuffer()->planes(), coverage, 24, 30, 45);
            BOOST_CHECK_LE(sharp, 2u);
            BOOST_CHECK_GE(blurred, 4u);
            BOOST_CHECK_LE(blurred, 7u);
        }
    }
}

/**
 * A quad sliding and a quad turning while the shutter is open.
 **/
BOOST_AUTO_TEST_CASE(moya_motion_reference_test) {
    v3d::moya::RenderContext rc;
    motionScene(rc);
    rc.render();

    check(rc.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS), MOTION, MOTION_RENDERED);
}

BOOST_AUTO_TEST_CASE(moya_motion_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(MOTION_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS),
        MOTION, MOTION_RIB_RENDERED);
}

/**
 * A quad translated across the shutter spreads over the distance it moved, its coverage
 * rising and falling linearly along it. The reyes hider moves the micropolygons to a sample's
 * time and the ray hider carries the ray back to where the quad was then.
 **/
BOOST_AUTO_TEST_CASE(moya_motion_spreads_linearly_test) {
    for (const char* hider : HIDERS) {
        BOOST_TEST_CONTEXT("hider " << hider) {
            v3d::moya::RenderContext rc;
            rc.hider(hider);
            slidingQuad(rc);
            rc.render();

            checkRamp(*rc.framebuffer()->planes(), v3d::moya::FrameBuffer::COVERAGE, 24);
        }
    }
}

/**
 * A quad showing a texture through paintedplastic, under each hider.
 *
 * The image's 2 by 2 texel blocks are each one colour, so a pixel well inside a block reads
 * that colour exactly whether it is sampled at the pixel or a micropolygon away from it. Both
 * hiders pin the same sixteen pixels: the same s and t, the same way up, and the same texel at
 * each. Under the reyes hider the quad is larger than a grid, so it splits, and its pieces
 * carry their st with them.
 **/
BOOST_AUTO_TEST_CASE(moya_textured_quad_test) {
    for (const char* hider : HIDERS) {
        BOOST_TEST_CONTEXT("hider " << hider) {
            boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = read("data/textured.rib", hider);

            // red counts the block across and green the block down, and blue is the same in each
            for (unsigned int across = 0; across < 4; across++) {
                for (unsigned int down = 0; down < 4; down++) {
                    const unsigned int column = 20 + 8 * across;
                    const unsigned int row = 12 + 8 * down;
                    BOOST_CHECK_SMALL(planes->value(v3d::moya::FrameBuffer::RED, column, row) - 85.0f * across / 255.0f,
                        1.0f / 255.0f);
                    BOOST_CHECK_SMALL(planes->value(v3d::moya::FrameBuffer::GREEN, column, row) - 85.0f * down / 255.0f,
                        1.0f / 255.0f);
                    BOOST_CHECK_SMALL(planes->value(v3d::moya::FrameBuffer::BLUE, column, row) - 128.0f / 255.0f,
                        1.0f / 255.0f);
                }
            }
            // and outside the quad is the background
            BOOST_CHECK_EQUAL(planes->value(v3d::moya::FrameBuffer::BLUE, 4, 4), 0.0f);
        }
    }
}

/**
 * A plastic panel casting a shadow across a matte floor under the reyes hider: the shadow is
 * traced through the shared scene, per ADR-0077, and falls where the ray hider's does.
 **/
BOOST_AUTO_TEST_CASE(moya_shadow_reference_from_rib_test) {
    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = read(SHADOW_SCENE);
    check(planes->image(v3d::moya::FrameBuffer::CHANNELS), SHADOW, SHADOW_RIB_RENDERED);

    /*
        And the two hiders agree on the same file away from the edges, where the reyes hider's
        micropolygons are flat: in a shadow of both lights, in the point light's shadow alone,
        lit by both, and the imager's background. An edge moves by up to a micropolygon, so
        only the insides are pinned.
    */
    boost::shared_ptr<v3d::render::offline::FrameBuffer> traced = read(SHADOW_SCENE, "raytrace");
    const unsigned int pixels[][2] = { { 40, 30 }, { 24, 40 }, { 56, 26 }, { 8, 8 }, { 2, 2 } };
    for (const auto & pixel : pixels) {
        BOOST_TEST_CONTEXT("pixel " << pixel[0] << ", " << pixel[1]) {
            for (unsigned int channel = 0; channel < 3; channel++) {
                BOOST_CHECK_SMALL(planes->value(channel, pixel[0], pixel[1]) - traced->value(channel, pixel[0], pixel[1]),
                    1.5f / 255.0f);
            }
        }
    }
}

/**
 * The ray hider's references, each reached by a file and by the render context, against one
 * committed picture. Two routes to one image: if they disagree, this says so, and neither of
 * them is the reference.
 **/
BOOST_AUTO_TEST_CASE(moya_raytrace_triangle_reference_test) {
    v3d::moya::RenderContext rc;
    triangleScene(rc);
    rc.render();
    check(picture(rc), TRIANGLE, TRIANGLE_RENDERED);
    check(read(TRIANGLE_SCENE)->image(v3d::moya::FrameBuffer::CHANNELS), TRIANGLE, TRIANGLE_RIB_RENDERED);
}

/**
 * A matte floor and a plastic panel, three lights of three kinds, and the panel's shadow
 * across the floor, one sample at each pixel centre.
 *
 * A shader is not tested by a picture - the language's own cases are in
 * v3dtest_render_offline. What this catches is the wiring: an ambient() that reached no light,
 * a shadow ray that started on the surface it left, an imager that ran over the wrong plane.
 **/
BOOST_AUTO_TEST_CASE(moya_raytrace_shaded_reference_test) {
    v3d::moya::RenderContext rc;
    raytraceShadedScene(rc);
    pixelCentres(rc);
    rc.render();
    check(picture(rc), TRACED_SHADED, TRACED_SHADED_RENDERED);
    check(read(TRACED_SHADED_SCENE)->image(v3d::moya::FrameBuffer::CHANNELS), TRACED_SHADED,
        TRACED_SHADED_RIB_RENDERED);
}

/**
 * The same scene at the RI defaults, two by two samples under a gaussian two pixels wide.
 **/
BOOST_AUTO_TEST_CASE(moya_raytrace_sampled_reference_test) {
    v3d::moya::RenderContext rc;
    raytraceShadedScene(rc);
    rc.render();
    check(picture(rc), TRACED_SAMPLED, TRACED_SAMPLED_RENDERED);
    check(read(TRACED_SAMPLED_SCENE)->image(v3d::moya::FrameBuffer::CHANNELS), TRACED_SAMPLED,
        TRACED_SAMPLED_RIB_RENDERED);
}

/**
 * Two renders are equal byte for byte, which is what lets a reference survive sampling at all.
 **/
BOOST_AUTO_TEST_CASE(moya_raytrace_render_is_repeatable_test) {
    v3d::moya::RenderContext first;
    raytraceShadedScene(first);
    first.render();
    v3d::moya::RenderContext second;
    raytraceShadedScene(second);
    second.render();

    const v3d::image::Difference difference = v3d::image::compare(*picture(first), *picture(second), 0);
    BOOST_CHECK_MESSAGE(difference.match, difference.description());
}

BOOST_AUTO_TEST_CASE(moya_raytrace_focus_reference_test) {
    v3d::moya::RenderContext rc;
    rc.hider("raytrace");
    focusScene(rc, true);
    rc.render();
    check(picture(rc), TRACED_FOCUS, TRACED_FOCUS_RENDERED);
    check(read(TRACED_FOCUS_SCENE)->image(v3d::moya::FrameBuffer::CHANNELS), TRACED_FOCUS,
        TRACED_FOCUS_RIB_RENDERED);
}

BOOST_AUTO_TEST_CASE(moya_raytrace_motion_reference_test) {
    v3d::moya::RenderContext rc;
    rc.hider("raytrace");
    motionScene(rc);
    rc.render();
    check(picture(rc), TRACED_MOTION, TRACED_MOTION_RENDERED);
    check(read(TRACED_MOTION_SCENE)->image(v3d::moya::FrameBuffer::CHANNELS), TRACED_MOTION,
        TRACED_MOTION_RIB_RENDERED);
}

/**
 * A metal sphere and a glass one over a checked floor: reflection, refraction, the fresnel
 * split between them, spheres, and a shadow through an occluder that is not opaque.
 **/
BOOST_AUTO_TEST_CASE(moya_raytrace_spheres_reference_test) {
    v3d::moya::RenderContext rc;
    spheresScene(rc);
    rc.render();
    check(picture(rc), SPHERES, SPHERES_RENDERED);
    check(read(SPHERES_SCENE)->image(v3d::moya::FrameBuffer::CHANNELS), SPHERES, SPHERES_RIB_RENDERED);
}

/**
 * A quad under one distant light forty five degrees off it, so a white quad comes out at the
 * cosine of that - a value neither the geometry's colour nor no shading could produce.
 **/
BOOST_AUTO_TEST_CASE(moya_raytrace_lit_quad_test) {
    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = read("data/raytrace-lit-quad.rib");

    BOOST_CHECK_CLOSE(planes->value(v3d::moya::FrameBuffer::RED, 32, 24), 0.70710678f, 0.5f);
    BOOST_CHECK_CLOSE(planes->value(v3d::moya::FrameBuffer::BLUE, 32, 24), 0.70710678f, 0.5f);
    // and nothing outside the quad
    BOOST_CHECK_SMALL(planes->value(v3d::moya::FrameBuffer::RED, 2, 2), 0.0001f);
}

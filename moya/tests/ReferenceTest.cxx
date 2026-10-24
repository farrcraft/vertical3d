/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Compare.h>
#include <api/image/Factory.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/Sampling.h>
#include <api/render/offline/rib/Declaration.h>
#include <api/render/offline/rib/Reader.h>
#include <moya/libmoya/Polygon.h>
#include <moya/libmoya/RIBHandler.h>
#include <moya/libmoya/RenderContext.h>
#include <moya/libmoya/Renderer.h>
#include <moya/libmoya/Vertex.h>

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

const char* BUMP = "data/reference-bump.png";
const char* BUMP_RENDERED = "data_out/reference-bump.png";
const char* BUMP_SCENE = "data/reference-bump.rib";
const char* BUMP_RIB_RENDERED = "data_out/reference-bump-rib.png";
const char* AREA = "data/reference-area.png";
const char* AREA_RENDERED = "data_out/reference-area.png";
const char* AREA_SCENE = "data/reference-area.rib";
const char* AREA_RIB_RENDERED = "data_out/reference-area-rib.png";
const char* TRACED_AREA = "data/raytrace-area.png";
const char* TRACED_AREA_RENDERED = "data_out/raytrace-area.png";
const char* TRACED_AREA_SCENE = "data/raytrace-area.rib";
const char* TRACED_AREA_RIB_RENDERED = "data_out/raytrace-area-rib.png";
const char* GLASS = "data/reference-glass.png";
const char* GLASS_RENDERED = "data_out/reference-glass.png";
const char* GLASS_SCENE = "data/reference-glass.rib";
const char* GLASS_RIB_RENDERED = "data_out/reference-glass-rib.png";
const char* DISPLACED = "data/reference-displaced.png";
const char* DISPLACED_RENDERED = "data_out/reference-displaced.png";
const char* DISPLACED_SCENE = "data/reference-displaced.rib";
const char* DISPLACED_RIB_RENDERED = "data_out/reference-displaced-rib.png";
const char* TRACED_DISPLACED = "data/raytrace-displaced.png";
const char* TRACED_DISPLACED_RENDERED = "data_out/raytrace-displaced.png";
const char* TRACED_DISPLACED_SCENE = "data/raytrace-displaced.rib";
const char* TRACED_DISPLACED_RIB_RENDERED = "data_out/raytrace-displaced-rib.png";
const char* TRACED_BUMP = "data/raytrace-bump.png";
const char* TRACED_BUMP_RENDERED = "data_out/raytrace-bump.png";
const char* TRACED_BUMP_SCENE = "data/raytrace-bump.rib";
const char* TRACED_BUMP_RIB_RENDERED = "data_out/raytrace-bump-rib.png";

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
 * A reference drawn this way checks the hider and the shading. reference-sampled checks the
 * sampling.
 **/
void pixelCentres(v3d::moya::RenderContext & rc) {
    rc.sampling().samples = glm::uvec2(1, 1);
    rc.sampling().filter = v3d::render::offline::Filter::Box;
    rc.sampling().width = glm::vec2(1.0F, 1.0F);
}

v3d::moya::Vertex vertex(float x, float y, float z) {
    v3d::moya::Vertex v;
    v.point(glm::vec3(x, y, z));
    return v;
}

/*
    A quad facing the camera, asymmetric about both axes so that a flipped picture is a
    failing one. The screen window is the default for a 4:3 frame, [-4/3, 4/3] by [-1, 1].
    The raster transform puts its origin at the upper left corner.

    Regenerate this reference when a change to shading or sampling alters the picture on
    purpose. Any other change to the picture fails the test.
*/
void scene(v3d::moya::RenderContext & rc) {
    rc.imageResolution({64, 48}, 1.0F);
    pixelCentres(rc);
    // the defaults are RI_EPSILON and RI_INFINITY, which leave the orthographic depth
    // scale at about 2e-38 and collapse every z onto the near plane
    rc.clipping({.hither = 1.0F, .yon = 100.0F});
    rc.prepareWorld();
    // the shader that means no shading, named rather than left as the default. The picture
    // shows that the shading language draws an unshaded surface unchanged
    rc.surface("constant", v3d::render::offline::rib::ParameterList());

    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.7F, -0.2F, 5.0F));
    polygon->addVertex(vertex(0.3F, -0.2F, 5.0F));
    polygon->addVertex(vertex(0.3F, 0.8F, 5.0F));
    polygon->addVertex(vertex(-0.7F, 0.8F, 5.0F));
    rc.addPolygon(polygon);
}

/**
 * A vertex with a shading normal of its own, so that a quad's cosine falloff is a gradient
 * rather than one value.
 **/
v3d::moya::Vertex shaded(float x, float y, float z, const glm::vec3 & normal) {
    v3d::moya::Vertex v;
    v.point(glm::vec3(x, y, z));
    v.normal(normal);
    return v;
}

void quad(v3d::moya::RenderContext & rc, float left, float right,
    const glm::vec3 & outer, const glm::vec3 & inner) {
    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(shaded(left, -0.8F, 5.0F, outer));
    polygon->addVertex(shaded(right, -0.8F, 5.0F, inner));
    polygon->addVertex(shaded(right, 0.8F, 5.0F, inner));
    polygon->addVertex(shaded(left, 0.8F, 5.0F, outer));
    rc.addPolygon(polygon);
}

/**
 * One parameter as a scene wrote it, typed the way the reader would have typed it.
 *
 * The list is an out parameter, as in rib::arguments. A ParameterList is a map, and returning
 * one by value copies every parameter in it.
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
    rc.imageResolution({64, 48}, 1.0F);
    rc.clipping({.hither = 1.0F, .yon = 100.0F});
    rc.prepareWorld();

    typedef v3d::render::offline::rib::Declaration Declaration;
    v3d::render::offline::rib::ParameterList distant;
    put(&distant, "intensity", Declaration::Type::FLOAT, { 0.8F });
    put(&distant, "to", Declaration::Type::POINT, { 0.6F, -0.4F, 1.0F });
    rc.lightSource({.shader = "distantlight", .handle = "1"}, distant);

    v3d::render::offline::rib::ParameterList bulb;
    put(&bulb, "intensity", Declaration::Type::FLOAT, { 4.0F });
    rc.pushTransform();
    rc.translate(-1.0F, 1.0F, 2.0F);
    rc.lightSource({.shader = "pointlight", .handle = "2"}, bulb);
    rc.popTransform();

    const glm::vec3 tilted(-0.9F, 0.0F, -0.436F);
    const glm::vec3 square(0.0F, 0.0F, -1.0F);

    rc.attributeBegin();
    rc.color(glm::vec3(0.9F, 0.4F, 0.2F));
    rc.surface("matte", v3d::render::offline::rib::ParameterList());
    quad(rc, -1.2F, -0.1F, tilted, square);
    rc.attributeEnd();

    rc.attributeBegin();
    rc.color(glm::vec3(0.3F, 0.5F, 0.9F));
    v3d::render::offline::rib::ParameterList plastic;
    put(&plastic, "roughness", Declaration::Type::FLOAT, { 0.08F });
    put(&plastic, "Ks", Declaration::Type::FLOAT, { 0.6F });
    rc.surface("plastic", plastic);
    quad(rc, 0.1F, 1.2F, square, glm::vec3(0.9F, 0.0F, -0.436F));
    rc.attributeEnd();
}

/*
    The same scene as data/reference-focus.rib: a red quad on the plane of focus four units
    out and a blue one ten units out, through a perspective camera. The lens is the file's,
    or a pinhole.
*/
void focusScene(v3d::moya::RenderContext & rc, bool lens) {
    rc.imageResolution({64, 48}, 1.0F);
    rc.sampling().samples = glm::uvec2(4, 4);
    if (lens) {
        rc.sampling().fstop = 1.0F;
        rc.sampling().focalLength = 0.5F;
        rc.sampling().focalDistance = 4.0F;
    }
    rc.projection("perspective", 40.0F);
    rc.clipping({.hither = 0.1F, .yon = 100.0F});
    rc.prepareWorld();
    rc.surface("constant", v3d::render::offline::rib::ParameterList());

    rc.color(glm::vec3(0.9F, 0.2F, 0.2F));
    const boost::shared_ptr<v3d::moya::Polygon> near = boost::make_shared<v3d::moya::Polygon>();
    near->addVertex(vertex(-1.5F, -1.0F, 4.0F));
    near->addVertex(vertex(-0.2F, -1.0F, 4.0F));
    near->addVertex(vertex(-0.2F, 1.0F, 4.0F));
    near->addVertex(vertex(-1.5F, 1.0F, 4.0F));
    rc.addPolygon(near);

    rc.color(glm::vec3(0.2F, 0.4F, 0.9F));
    const boost::shared_ptr<v3d::moya::Polygon> far = boost::make_shared<v3d::moya::Polygon>();
    far->addVertex(vertex(0.5F, -2.5F, 10.0F));
    far->addVertex(vertex(4.0F, -2.5F, 10.0F));
    far->addVertex(vertex(4.0F, 2.5F, 10.0F));
    far->addVertex(vertex(0.5F, 2.5F, 10.0F));
    rc.addPolygon(far);
}

/** A row, and the columns along it from the first to the last, inclusive. **/
struct Run {
    unsigned int row = 0;
    unsigned int from = 0;
    unsigned int to = 0;
};

/**
 * How many pixels along a row are partly covered. That count is the width of an edge.
 **/
unsigned int partial(const v3d::render::offline::FrameBuffer & planes, unsigned int coverage, const Run & run) {
    unsigned int count = 0;
    for (unsigned int column = run.from; column <= run.to; column++) {
        const float value = planes.value(coverage, column, run.row);
        if (value > 0.02F && value < 0.98F) {
            count++;
        }
    }
    return count;
}

/** The first and last columns of a block, inclusive. **/
struct Columns {
    unsigned int from = 0;
    unsigned int to = 0;
};

/**
 * The largest difference between two frames over every plane of a block of columns.
 **/
float largest(const v3d::render::offline::FrameBuffer & first, const v3d::render::offline::FrameBuffer & second,
    const Columns & columns) {
    float worst = 0.0F;
    for (unsigned int plane = 0; plane < first.planes(); plane++) {
        for (unsigned int row = 0; row < first.height(); row++) {
            for (unsigned int column = columns.from; column <= columns.to; column++) {
                worst = std::max(worst, std::fabs(first.value(plane, column, row) - second.value(plane, column, row)));
            }
        }
    }
    return worst;
}

/**
 * The orthographic camera of the other references, with the shutter open from 0 to 1.
 **/
void motionCamera(v3d::moya::RenderContext & rc) {
    rc.imageResolution({64, 48}, 1.0F);
    rc.sampling().samples = glm::uvec2(4, 4);
    rc.sampling().shutter = glm::vec2(0.0F, 1.0F);
    rc.clipping({.hither = 0.1F, .yon = 100.0F});
    rc.prepareWorld();
    rc.surface("constant", v3d::render::offline::rib::ParameterList());
}

void quadAt(v3d::moya::RenderContext & rc, const glm::vec3 (&corners)[4]) {
    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
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
    rc.motionBegin({ 0.0F, 1.0F });
    rc.translate(0.0F, 0.0F, 0.0F);
    rc.translate(0.5F, 0.0F, 0.0F);
    rc.motionEnd();
    rc.color(glm::vec3(0.9F, 0.8F, 0.2F));
    const glm::vec3 slid[4] = {
        glm::vec3(-1.2F, 0.1F, 5.0F), glm::vec3(-0.4F, 0.1F, 5.0F),
        glm::vec3(-0.4F, 0.8F, 5.0F), glm::vec3(-1.2F, 0.8F, 5.0F)
    };
    quadAt(rc, slid);
    rc.attributeEnd();

    rc.attributeBegin();
    rc.translate(0.6F, -0.4F, 5.0F);
    rc.motionBegin({ 0.0F, 1.0F });
    rc.rotate(0.0F, {0.0F, 0.0F, 1.0F});
    rc.rotate(90.0F, {0.0F, 0.0F, 1.0F});
    rc.motionEnd();
    rc.color(glm::vec3(0.2F, 0.7F, 0.9F));
    const glm::vec3 turned[4] = {
        glm::vec3(-0.35F, -0.15F, 0.0F), glm::vec3(0.35F, -0.15F, 0.0F),
        glm::vec3(0.35F, 0.15F, 0.0F), glm::vec3(-0.35F, 0.15F, 0.0F)
    };
    quadAt(rc, turned);
    rc.attributeEnd();
}

/**
 * The coverage of a quad that slid twelve pixels right while the shutter was open, along the
 * row through its middle. Its left edge leaves pixels 8 to 19 one after another, and its
 * right edge reaches pixels 32 to 43. Under a one pixel box the coverage climbs a twelfth a
 * pixel from 8, falls a twelfth a pixel from 32, and is whole between.
 **/
void checkRamp(const v3d::render::offline::FrameBuffer & planes, unsigned int coverage, unsigned int row) {
    BOOST_CHECK_EQUAL(planes.value(coverage, 7, row), 0.0F);
    for (unsigned int k = 0; k < 12; k++) {
        BOOST_TEST_CONTEXT("pixel " << 8 + k << " and " << 32 + k) {
            const float rising = (static_cast<float>(k) + 0.5F) / 12.0F;
            BOOST_CHECK_SMALL(planes.value(coverage, 8 + k, row) - rising, 0.1F);
            BOOST_CHECK_SMALL(planes.value(coverage, 32 + k, row) - (1.0F - rising), 0.1F);
        }
    }
    BOOST_CHECK_CLOSE(planes.value(coverage, 25, row), 1.0F, 1.0e-4F);
    BOOST_CHECK_EQUAL(planes.value(coverage, 44, row), 0.0F);
}

/**
 * A rendered image against a committed one, writing what it rendered when they differ.
 **/
void check(const boost::shared_ptr<v3d::image::Image> & rendered, const char* reference,
    const char* output) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);
    const boost::shared_ptr<v3d::image::Image> committed = factory.read(reference);
    if (committed == nullptr) {
        boost::filesystem::create_directory("data_out");
        factory.write(output, rendered);
    }
    BOOST_REQUIRE_MESSAGE(committed != nullptr,
        std::string("cannot read the reference image ") + reference +
        " - what was rendered is in " + output);

    const v3d::image::Difference difference = v3d::image::compare(*rendered, *committed, 1);
    if (!difference.match) {
        boost::filesystem::create_directory("data_out");
        factory.write(output, rendered);
    }
    BOOST_CHECK_MESSAGE(difference.match,
        difference.description() + " - what was rendered instead is in " + output);
}

/** A scene file, and the hider to render it under, or null for the one the file names. **/
struct Scene {
    const char* file = nullptr;
    const char* hider = nullptr;
};

/**
 * A frame read from a file, under the hider the file names unless the caller names one first.
 **/
boost::shared_ptr<v3d::render::offline::FrameBuffer> read(const Scene & scene) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    if (scene.hider != nullptr) {
        handler.context().hider(scene.hider);
    }
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());
    BOOST_REQUIRE(reader.read(scene.file, &handler));
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
    rc.imageResolution({64, 48}, 1.0F);
    rc.clipping({.hither = 0.001F, .yon = 100.0F});
    rc.projection("orthographic");
    rc.setTransform(glm::translate(glm::mat4x4(1.0F), glm::vec3(0.0F, 0.0F, 1.0F)));
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
    background(rc, glm::vec3(0.15F, 0.25F, 0.45F));
    rc.prepareWorld();
    rc.surface("constant", v3d::render::offline::rib::ParameterList());
    rc.color(glm::vec3(0.9F, 0.2F, 0.2F));
    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.8F, -0.6F, 2.0F));
    polygon->addVertex(vertex(0.8F, -0.6F, 2.0F));
    polygon->addVertex(vertex(0.0F, 0.7F, 2.0F));
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
    background(rc, glm::vec3(0.05F, 0.06F, 0.1F));
    rc.prepareWorld();

    v3d::render::offline::rib::ParameterList fill;
    put(&fill, "intensity", Declaration::Type::FLOAT, { 0.18F });
    rc.lightSource({.shader = "ambientlight", .handle = "0"}, fill);
    v3d::render::offline::rib::ParameterList distant;
    put(&distant, "intensity", Declaration::Type::FLOAT, { 0.9F });
    put(&distant, "to", Declaration::Type::POINT, { 0.7F, -0.7F, 1.0F });
    rc.lightSource({.shader = "distantlight", .handle = "1"}, distant);
    v3d::render::offline::rib::ParameterList lamp;
    put(&lamp, "intensity", Declaration::Type::FLOAT, { 1.2F });
    rc.pushTransform();
    rc.translate(-0.6F, 0.5F, 1.2F);
    rc.lightSource({.shader = "pointlight", .handle = "2"}, lamp);
    rc.popTransform();

    rc.attributeBegin();
    rc.color(glm::vec3(0.8F, 0.75F, 0.6F));
    rc.surface("matte", v3d::render::offline::rib::ParameterList());
    const glm::vec3 floor[4] = {
        glm::vec3(-1.15F, -1.2F, 3.0F), glm::vec3(1.15F, -1.2F, 3.0F),
        glm::vec3(1.15F, 1.2F, 3.0F), glm::vec3(-1.15F, 1.2F, 3.0F)
    };
    quadAt(rc, floor);
    rc.attributeEnd();

    rc.attributeBegin();
    rc.color(glm::vec3(0.2F, 0.45F, 0.8F));
    v3d::render::offline::rib::ParameterList shiny;
    put(&shiny, "roughness", Declaration::Type::FLOAT, { 0.1F });
    put(&shiny, "Ks", Declaration::Type::FLOAT, { 0.5F });
    rc.surface("plastic", shiny);
    const glm::vec3 panel[4] = {
        glm::vec3(-0.5F, -0.45F, 2.2F), glm::vec3(0.35F, -0.45F, 2.2F),
        glm::vec3(0.35F, 0.4F, 2.2F), glm::vec3(-0.5F, 0.4F, 2.2F)
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
    rc.imageResolution({64, 48}, 1.0F);
    rc.clipping({.hither = 0.1F, .yon = 100.0F});
    rc.projection("perspective", 40.0F);
    // the floor's shader is a fixture beside the scene rather than one of the standard ones
    rc.searchpath("data:&");
    rc.prepareWorld();

    v3d::render::offline::rib::ParameterList fill;
    put(&fill, "intensity", Declaration::Type::FLOAT, { 0.2F });
    rc.lightSource({.shader = "ambientlight", .handle = "0"}, fill);
    v3d::render::offline::rib::ParameterList distant;
    put(&distant, "intensity", Declaration::Type::FLOAT, { 0.9F });
    put(&distant, "to", Declaration::Type::POINT, { 0.4F, -1.0F, 0.6F });
    rc.lightSource({.shader = "distantlight", .handle = "1"}, distant);

    rc.attributeBegin();
    rc.color(glm::vec3(0.85F, 0.8F, 0.7F));
    v3d::render::offline::rib::ParameterList checks;
    put(&checks, "size", Declaration::Type::FLOAT, { 0.5F });
    rc.surface("checked", checks);
    const glm::vec3 floor[4] = {
        glm::vec3(-4.0F, -1.0F, 2.0F), glm::vec3(4.0F, -1.0F, 2.0F),
        glm::vec3(4.0F, -1.0F, 12.0F), glm::vec3(-4.0F, -1.0F, 12.0F)
    };
    quadAt(rc, floor);
    rc.attributeEnd();

    rc.attributeBegin();
    rc.color(glm::vec3(0.35F, 0.5F, 0.75F));
    rc.surface("matte", v3d::render::offline::rib::ParameterList());
    const glm::vec3 wall[4] = {
        glm::vec3(-4.0F, -1.0F, 12.0F), glm::vec3(4.0F, -1.0F, 12.0F),
        glm::vec3(4.0F, 4.0F, 12.0F), glm::vec3(-4.0F, 4.0F, 12.0F)
    };
    quadAt(rc, wall);
    rc.attributeEnd();

    rc.attributeBegin();
    rc.color(glm::vec3(0.9F, 0.85F, 0.7F));
    v3d::render::offline::rib::ParameterList mirror;
    put(&mirror, "Ka", Declaration::Type::FLOAT, { 0.1F });
    put(&mirror, "Ks", Declaration::Type::FLOAT, { 0.6F });
    put(&mirror, "Kr", Declaration::Type::FLOAT, { 0.8F });
    rc.surface("shinymetal", mirror);
    rc.translate(-1.1F, -0.28F, 6.0F);
    BOOST_REQUIRE(rc.addSphere({.radius = 0.7F, .zmin = -0.7F, .zmax = 0.7F, .thetamax = 360.0F}));
    rc.attributeEnd();

    rc.attributeBegin();
    rc.color(glm::vec3(1.0F));
    rc.opacity(glm::vec3(0.3F));
    rc.surface("glass", v3d::render::offline::rib::ParameterList());
    rc.translate(1.1F, -0.28F, 5.5F);
    BOOST_REQUIRE(rc.addSphere({.radius = 0.7F, .zmin = -0.7F, .zmax = 0.7F, .thetamax = 360.0F}));
    rc.attributeEnd();
}

/**
 * A quad sliding half a unit right across the whole shutter, sampled finely under a one pixel
 * box, which is twelve pixels at this frame.
 **/
void slidingQuad(v3d::moya::RenderContext & rc) {
    rc.imageResolution({64, 48}, 1.0F);
    rc.sampling().samples = glm::uvec2(8, 8);
    rc.sampling().filter = v3d::render::offline::Filter::Box;
    rc.sampling().width = glm::vec2(1.0F);
    rc.sampling().shutter = glm::vec2(0.0F, 1.0F);
    rc.clipping({.hither = 0.1F, .yon = 100.0F});
    rc.prepareWorld();
    rc.surface("constant", v3d::render::offline::rib::ParameterList());

    rc.motionBegin({ 0.0F, 1.0F });
    rc.translate(0.0F, 0.0F, 0.0F);
    rc.translate(0.5F, 0.0F, 0.0F);
    rc.motionEnd();
    const glm::vec3 corners[4] = {
        glm::vec3(-1.0F, -0.5F, 5.0F), glm::vec3(0.0F, -0.5F, 5.0F),
        glm::vec3(0.0F, 0.5F, 5.0F), glm::vec3(-1.0F, 0.5F, 5.0F)
    };
    quadAt(rc, corners);
}

/**
 * Rendering the buckets writes samples where the polygon is, and leaves the rest of the planes
 * at what they were allocated with.
 **/
BOOST_AUTO_TEST_CASE(moya_renders_a_polygon_test) {
    v3d::moya::RenderContext rc;
    scene(rc);
    rc.render();

    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = rc.framebuffer()->planes();

    // the quad covers raster x over [15.2, 39.2] and y over [4.8, 28.8]
    BOOST_TEST(planes->value(v3d::moya::FrameBuffer::RED, 24, 16) == 1.0F);
    BOOST_TEST(planes->value(v3d::moya::FrameBuffer::BLUE, 24, 16) == 1.0F);
    // and nothing outside it
    BOOST_TEST(planes->value(v3d::moya::FrameBuffer::RED, 55, 40) == 0.0F);

    // a covered pixel took a depth from the geometry rather than keeping the one it was
    // cleared to
    BOOST_TEST(planes->value(v3d::moya::FrameBuffer::DEPTH, 24, 16) < 2.0F);
}

/**
 * A rendered image against a committed one. moya touches no window, device or swapchain,
 * so this runs on any machine.
 **/
BOOST_AUTO_TEST_CASE(moya_reference_test) {
    v3d::moya::RenderContext rc;
    scene(rc);
    rc.render();

    const boost::shared_ptr<v3d::image::Image> rendered =
        rc.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS);

    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    const boost::shared_ptr<v3d::image::Image> reference = factory.read(REFERENCE);
    if (reference == nullptr) {
        // write what was rendered, so a missing reference can be inspected, or committed when
        // it is being regenerated on purpose
        boost::filesystem::create_directory("data_out");
        factory.write(RENDERED, rendered);
    }
    BOOST_REQUIRE_MESSAGE(reference != nullptr,
        std::string("cannot read the reference image ") + REFERENCE + " - what was rendered is in " + RENDERED);

    // the tolerance is for float rounding across compilers, not for "close enough"
    const v3d::image::Difference difference = v3d::image::compare(*rendered, *reference, 1);
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
 * The file and the code are two routes to one image, and neither route is the reference. If
 * the two disagree, this case fails. A fault in the reader shows as a wrong picture.
 **/
BOOST_AUTO_TEST_CASE(moya_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(RIB_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    const boost::shared_ptr<v3d::image::Image> rendered =
        handler.context().framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS);

    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);
    const boost::shared_ptr<v3d::image::Image> reference = factory.read(REFERENCE);
    BOOST_REQUIRE(reference != nullptr);

    const v3d::image::Difference difference = v3d::image::compare(*rendered, *reference, 1);
    if (!difference.match) {
        boost::filesystem::create_directory("data_out");
        factory.write(RIB_RENDERED, rendered);
    }
    BOOST_CHECK_MESSAGE(difference.match,
        difference.description() + " - what the rib scene rendered instead is in " + RIB_RENDERED);
}

/**
 * A display of type "file" writes the sampled planes to a picture on disk. The driver's
 * --output reaches the context through RiDisplay, and nothing else writes a file.
 **/
BOOST_AUTO_TEST_CASE(moya_display_writes_a_file_test) {
    boost::filesystem::create_directory("data_out");
    const char* output = "data_out/display.png";
    boost::filesystem::remove(output);

    v3d::moya::RenderContext rc;
    scene(rc);
    rc.display({.name = output, .type = "file", .mode = "rgb"});
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

    BOOST_TEST(rc.framebuffer()->planes()->value(v3d::moya::FrameBuffer::RED, 24, 16) == 1.0F);
}

/**
 * A shaded scene: a matte surface and a plastic one, a distant light and a point light. Each
 * surface carries normals of its own, so the falloff is a gradient.
 *
 * The picture does not test individual shaders, which have their own cases in
 * v3dtest_render_offline. This case checks the wiring between the shading language and
 * the renderer.
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
 * The file and the code are two routes to one image. A scene with shaders has more for the two
 * routes to disagree about. A shader parameter bound on one route and defaulted on the other
 * shows here.
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
 * The shaded scene at the RI defaults, two by two samples under a gaussian two pixels wide.
 * Its edges are antialiased, and it is the same on every run because every pixel's samples
 * are seeded by where the pixel is.
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

            BOOST_CHECK_SMALL(largest(*pinhole.framebuffer()->planes(), *lens.framebuffer()->planes(), {.from = 0, .to = 30}),
                1.0F / 255.0F);
        }
    }
}

/**
 * The quad off the plane of focus spreads its edge over its circle of confusion. Under a one
 * pixel box, a pinhole leaves the edge in a pixel or two. The lens has a radius of a quarter
 * of a unit and is focused four units out. It blurs a point ten units out over
 * 2 * 0.25 * (10 - 4) / 10 of a unit, measured on the plane of focus. That is about five
 * pixels at this field of view. The reyes hider blurs by moving the micropolygon, and the
 * ray hider by moving the ray.
 **/
BOOST_AUTO_TEST_CASE(moya_out_of_focus_spreads_test) {
    for (const char* hider : HIDERS) {
        BOOST_TEST_CONTEXT("hider " << hider) {
            v3d::moya::RenderContext pinhole;
            pinhole.hider(hider);
            focusScene(pinhole, false);
            pinhole.sampling().filter = v3d::render::offline::Filter::Box;
            pinhole.sampling().width = glm::vec2(1.0F);
            pinhole.render();
            v3d::moya::RenderContext lens;
            lens.hider(hider);
            focusScene(lens, true);
            lens.sampling().filter = v3d::render::offline::Filter::Box;
            lens.sampling().width = glm::vec2(1.0F);
            lens.render();

            const unsigned int coverage = v3d::moya::FrameBuffer::COVERAGE;
            const unsigned int sharp = partial(*pinhole.framebuffer()->planes(), coverage, {.row = 24, .from = 30, .to = 45});
            const unsigned int blurred = partial(*lens.framebuffer()->planes(), coverage, {.row = 24, .from = 30, .to = 45});
            BOOST_CHECK_LE(sharp, 2U);
            BOOST_CHECK_GE(blurred, 4U);
            BOOST_CHECK_LE(blurred, 7U);
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
 * A quad translated across the shutter spreads over the distance it moved. Its coverage rises
 * and falls linearly along that distance. The reyes hider moves the micropolygons to a
 * sample's time, and the ray hider carries the ray back to where the quad was then.
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
 * that colour exactly whether it is sampled at the pixel or a micropolygon away from it. The
 * case checks a grid of pixels under both hiders: the same s and t, the same way up, and the
 * same texel at each. Under the reyes hider the quad is larger than a grid, so it splits, and
 * its pieces carry their st with them.
 **/
BOOST_AUTO_TEST_CASE(moya_textured_quad_test) {
    for (const char* hider : HIDERS) {
        BOOST_TEST_CONTEXT("hider " << hider) {
            const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = read({.file = "data/textured.rib", .hider = hider});

            // red counts the block across and green the block down, and blue is the same in each
            for (unsigned int across = 0; across < 4; across++) {
                for (unsigned int down = 0; down < 4; down++) {
                    const unsigned int column = 20 + (8 * across);
                    const unsigned int row = 12 + (8 * down);
                    BOOST_CHECK_SMALL(planes->value(v3d::moya::FrameBuffer::RED, column, row) - (85.0F * across / 255.0F),
                        1.0F / 255.0F);
                    BOOST_CHECK_SMALL(planes->value(v3d::moya::FrameBuffer::GREEN, column, row) - (85.0F * down / 255.0F),
                        1.0F / 255.0F);
                    BOOST_CHECK_SMALL(planes->value(v3d::moya::FrameBuffer::BLUE, column, row) - (128.0F / 255.0F),
                        1.0F / 255.0F);
                }
            }
            // and outside the quad is the background
            BOOST_CHECK_EQUAL(planes->value(v3d::moya::FrameBuffer::BLUE, 4, 4), 0.0F);
        }
    }
}

/**
 * A plastic panel casting a shadow across a matte floor under the reyes hider. The shadow is
 * traced through the same scene the ray hider uses, so it falls where the ray hider's does.
 **/
BOOST_AUTO_TEST_CASE(moya_shadow_reference_from_rib_test) {
    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = read({.file = SHADOW_SCENE});
    check(planes->image(v3d::moya::FrameBuffer::CHANNELS), SHADOW, SHADOW_RIB_RENDERED);

    /*
        The two hiders also agree on the same file away from the edges, where the reyes hider's
        micropolygons are flat. The regions checked are a shadow of both lights, the point
        light's shadow alone, a region lit by both, and the imager's background. An edge moves
        by up to a micropolygon, so only the insides are checked.
    */
    const boost::shared_ptr<v3d::render::offline::FrameBuffer> traced = read({.file = SHADOW_SCENE, .hider = "raytrace"});
    const unsigned int pixels[][2] = { { 40, 30 }, { 24, 40 }, { 56, 26 }, { 8, 8 }, { 2, 2 } };
    for (const auto & pixel : pixels) {
        BOOST_TEST_CONTEXT("pixel " << pixel[0] << ", " << pixel[1]) {
            for (unsigned int channel = 0; channel < 3; channel++) {
                BOOST_CHECK_SMALL(planes->value(channel, pixel[0], pixel[1]) - traced->value(channel, pixel[0], pixel[1]),
                    1.5F / 255.0F);
            }
        }
    }
}

/**
 * The ray hider's references, each rendered from a file and through the render context, against
 * one committed picture. Neither route is the reference, and the case fails if they disagree.
 **/
BOOST_AUTO_TEST_CASE(moya_raytrace_triangle_reference_test) {
    v3d::moya::RenderContext rc;
    triangleScene(rc);
    rc.render();
    check(picture(rc), TRIANGLE, TRIANGLE_RENDERED);
    check(read({.file = TRIANGLE_SCENE})->image(v3d::moya::FrameBuffer::CHANNELS), TRIANGLE, TRIANGLE_RIB_RENDERED);
}

/**
 * A matte floor and a plastic panel, three lights of three kinds, and the panel's shadow
 * across the floor, one sample at each pixel centre.
 *
 * The picture does not test individual shaders, which have their own cases in
 * v3dtest_render_offline. This case checks the wiring between the shaders and the ray hider.
 * It fails if ambient() reaches no light, if a shadow ray starts on the surface it leaves, or if
 * an imager runs over the wrong plane.
 **/
BOOST_AUTO_TEST_CASE(moya_raytrace_shaded_reference_test) {
    v3d::moya::RenderContext rc;
    raytraceShadedScene(rc);
    pixelCentres(rc);
    rc.render();
    check(picture(rc), TRACED_SHADED, TRACED_SHADED_RENDERED);
    check(read({.file = TRACED_SHADED_SCENE})->image(v3d::moya::FrameBuffer::CHANNELS), TRACED_SHADED,
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
    check(read({.file = TRACED_SAMPLED_SCENE})->image(v3d::moya::FrameBuffer::CHANNELS), TRACED_SAMPLED,
        TRACED_SAMPLED_RIB_RENDERED);
}

/**
 * Two renders are equal byte for byte. A sampled picture can have a reference only if they are.
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
    check(read({.file = TRACED_FOCUS_SCENE})->image(v3d::moya::FrameBuffer::CHANNELS), TRACED_FOCUS,
        TRACED_FOCUS_RIB_RENDERED);
}

BOOST_AUTO_TEST_CASE(moya_raytrace_motion_reference_test) {
    v3d::moya::RenderContext rc;
    rc.hider("raytrace");
    motionScene(rc);
    rc.render();
    check(picture(rc), TRACED_MOTION, TRACED_MOTION_RENDERED);
    check(read({.file = TRACED_MOTION_SCENE})->image(v3d::moya::FrameBuffer::CHANNELS), TRACED_MOTION,
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
    check(read({.file = SPHERES_SCENE})->image(v3d::moya::FrameBuffer::CHANNELS), SPHERES, SPHERES_RIB_RENDERED);
}

/**
 * A white quad under one distant light forty five degrees off its normal comes out at the
 * cosine of that angle. An unshaded quad would come out white.
 **/
BOOST_AUTO_TEST_CASE(moya_raytrace_lit_quad_test) {
    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = read({.file = "data/raytrace-lit-quad.rib"});

    BOOST_CHECK_CLOSE(planes->value(v3d::moya::FrameBuffer::RED, 32, 24), 0.70710678F, 0.5F);
    BOOST_CHECK_CLOSE(planes->value(v3d::moya::FrameBuffer::BLUE, 32, 24), 0.70710678F, 0.5F);
    // and nothing outside the quad
    BOOST_CHECK_SMALL(planes->value(v3d::moya::FrameBuffer::RED, 2, 2), 0.0001F);
}

/**
 * A moving grid that sweeps more samples than the motion cache holds works out each
 * micropolygon's motion afresh, and draws the same picture as one that caches them.
 **/
BOOST_AUTO_TEST_CASE(reference_motion_without_a_cache_test) {
    v3d::moya::RenderContext cached;
    motionScene(cached);
    cached.render();

    v3d::moya::RenderContext uncached;
    uncached.motionCache(0);
    motionScene(uncached);
    uncached.render();

    const v3d::render::offline::FrameBuffer & cachedImage = *cached.framebuffer()->planes();
    const v3d::render::offline::FrameBuffer & uncachedImage = *uncached.framebuffer()->planes();
    BOOST_CHECK_EQUAL(largest(cachedImage, uncachedImage, {.from = 0, .to = cachedImage.width() - 1}), 0.0F);
}

/*
    The same scene as data/reference-bump.rib, and as data/raytrace-bump.rib under the ray
    hider. A plane faces the camera, bumped by noise in its own space and lit low across its
    face, so the bumps show as light and shade. A plane with no bumps would be one flat colour.
*/
void bumpScene(v3d::moya::RenderContext & rc, const char* hider) {
    rc.hider(hider);
    rc.imageResolution({64, 48}, 1.0F);
    pixelCentres(rc);
    rc.clipping({.hither = 1.0F, .yon = 100.0F});
    rc.searchpath("data:&");
    rc.prepareWorld();

    typedef v3d::render::offline::rib::Declaration Declaration;
    v3d::render::offline::rib::ParameterList ambient;
    put(&ambient, "intensity", Declaration::Type::FLOAT, { 0.1F });
    rc.lightSource({.shader = "ambientlight", .handle = "0"}, ambient);
    v3d::render::offline::rib::ParameterList low;
    put(&low, "intensity", Declaration::Type::FLOAT, { 1.0F });
    put(&low, "to", Declaration::Type::POINT, { 0.8F, -0.3F, 0.6F });
    rc.lightSource({.shader = "distantlight", .handle = "1"}, low);

    rc.color(glm::vec3(0.85F, 0.8F, 0.7F));
    rc.surface("bumpy", v3d::render::offline::rib::ParameterList());
    const boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-1.2F, -0.9F, 5.0F));
    polygon->addVertex(vertex(-1.2F, 0.9F, 5.0F));
    polygon->addVertex(vertex(1.2F, 0.9F, 5.0F));
    polygon->addVertex(vertex(1.2F, -0.9F, 5.0F));
    rc.addPolygon(polygon);
}

/**
 * A bumped plane under the reyes hider. calculatenormal differences the bumped points across
 * each grid, so the shading follows the bumps.
 **/
BOOST_AUTO_TEST_CASE(moya_bump_reference_test) {
    v3d::moya::RenderContext rc;
    bumpScene(rc, "hidden");
    rc.render();

    check(rc.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS), BUMP, BUMP_RENDERED);
}

/**
 * The same bumped plane said in RIB, against the same committed picture.
 **/
BOOST_AUTO_TEST_CASE(moya_bump_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(BUMP_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS),
        BUMP, BUMP_RIB_RENDERED);
}

/**
 * The bumped plane under the ray hider. Each hit is shaded with two neighbours a pixel away,
 * so calculatenormal follows the bumps here too. The hiders sample differently, so this
 * picture is not compared with reference-bump.png.
 **/
BOOST_AUTO_TEST_CASE(moya_traced_bump_reference_test) {
    v3d::moya::RenderContext rc;
    bumpScene(rc, "raytrace");
    rc.render();

    check(rc.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS), TRACED_BUMP, TRACED_BUMP_RENDERED);
}

/**
 * The same traced bumped plane said in RIB, against the same committed picture.
 **/
BOOST_AUTO_TEST_CASE(moya_traced_bump_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(TRACED_BUMP_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS),
        TRACED_BUMP, TRACED_BUMP_RIB_RENDERED);
}

/*
    The same scene as data/reference-area.rib, and as data/raytrace-area.rib under the ray
    hider. A box stands in front of a wall, lit by a quad light up and to its left. The light
    is sampled at 16 points for every shading point, so the box's shadow on the wall is soft.
    The light's quad is drawn too, in white.
*/
void areaScene(v3d::moya::RenderContext & rc, const char* hider) {
    rc.hider(hider);
    rc.imageResolution({64, 48}, 1.0F);
    pixelCentres(rc);
    rc.screenWindow({.left = -2.0F, .right = 2.0F, .bottom = -1.5F, .top = 1.5F});
    rc.clipping({.hither = 0.1F, .yon = 100.0F});
    rc.prepareWorld();

    typedef v3d::render::offline::rib::Declaration Declaration;
    v3d::render::offline::rib::ParameterList ambient;
    put(&ambient, "intensity", Declaration::Type::FLOAT, { 0.08F });
    rc.lightSource({.shader = "ambientlight", .handle = "0"}, ambient);

    rc.attributeBegin();
    v3d::render::offline::rib::ParameterList count;
    put(&count, "nsamples", Declaration::Type::INTEGER, { 16.0F });
    rc.attribute("light", count);
    v3d::render::offline::rib::ParameterList bright;
    put(&bright, "intensity", Declaration::Type::FLOAT, { 6.0F });
    rc.areaLightSource({.shader = "arealight", .handle = "1"}, bright);
    rc.surface("constant", v3d::render::offline::rib::ParameterList());
    quadAt(rc, {glm::vec3(-1.9F, 0.6F, 3.0F), glm::vec3(-0.9F, 0.6F, 3.0F), glm::vec3(-0.9F, 1.4F, 3.0F),
        glm::vec3(-1.9F, 1.4F, 3.0F)});
    rc.attributeEnd();
    // the block switched the light off, as it does any light made in one
    rc.illuminate("1", true);

    rc.attributeBegin();
    rc.color(glm::vec3(0.8F, 0.8F, 0.75F));
    rc.surface("matte", v3d::render::offline::rib::ParameterList());
    quadAt(rc, {glm::vec3(-2.0F, -1.5F, 6.0F), glm::vec3(-2.0F, 1.5F, 6.0F), glm::vec3(2.0F, 1.5F, 6.0F),
        glm::vec3(2.0F, -1.5F, 6.0F)});
    rc.attributeEnd();

    // the box from (-0.2, -0.6, 4.6) to (0.6, 0.2, 5.4), each face wound to face out
    rc.attributeBegin();
    rc.color(glm::vec3(0.7F, 0.3F, 0.25F));
    rc.surface("matte", v3d::render::offline::rib::ParameterList());
    const float x0 = -0.2F;
    const float x1 = 0.6F;
    const float y0 = -0.6F;
    const float y1 = 0.2F;
    const float z0 = 4.6F;
    const float z1 = 5.4F;
    quadAt(rc, {glm::vec3(x0, y0, z0), glm::vec3(x0, y1, z0), glm::vec3(x1, y1, z0), glm::vec3(x1, y0, z0)});
    quadAt(rc, {glm::vec3(x1, y0, z1), glm::vec3(x1, y1, z1), glm::vec3(x0, y1, z1), glm::vec3(x0, y0, z1)});
    quadAt(rc, {glm::vec3(x0, y0, z1), glm::vec3(x0, y1, z1), glm::vec3(x0, y1, z0), glm::vec3(x0, y0, z0)});
    quadAt(rc, {glm::vec3(x1, y0, z0), glm::vec3(x1, y1, z0), glm::vec3(x1, y1, z1), glm::vec3(x1, y0, z1)});
    quadAt(rc, {glm::vec3(x0, y1, z0), glm::vec3(x0, y1, z1), glm::vec3(x1, y1, z1), glm::vec3(x1, y1, z0)});
    quadAt(rc, {glm::vec3(x0, y0, z1), glm::vec3(x0, y0, z0), glm::vec3(x1, y0, z0), glm::vec3(x1, y0, z1)});
    rc.attributeEnd();
}

/**
 * A soft shadow from an area light under the reyes hider. Each grid vertex runs the light
 * at 16 points on its quad, each with its own shadow ray, so the shadow's edge is a gradient.
 **/
BOOST_AUTO_TEST_CASE(moya_area_reference_test) {
    v3d::moya::RenderContext rc;
    areaScene(rc, "hidden");
    rc.render();

    check(rc.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS), AREA, AREA_RENDERED);
}

/**
 * The same area light said in RIB, against the same committed picture.
 **/
BOOST_AUTO_TEST_CASE(moya_area_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(AREA_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS), AREA, AREA_RIB_RENDERED);
}

/**
 * The soft shadow under the ray hider. It is not compared with reference-area.png, because the
 * hiders sample differently.
 **/
BOOST_AUTO_TEST_CASE(moya_traced_area_reference_test) {
    v3d::moya::RenderContext rc;
    areaScene(rc, "raytrace");
    rc.render();

    check(rc.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS), TRACED_AREA, TRACED_AREA_RENDERED);
}

/**
 * The traced area light said in RIB, against the same committed picture.
 **/
BOOST_AUTO_TEST_CASE(moya_traced_area_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(TRACED_AREA_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS),
        TRACED_AREA, TRACED_AREA_RIB_RENDERED);
}

/*
    The same scene as data/reference-glass.rib. A blue quad of opacity one half stands in front
    of a checked plane, which is tilted away from the camera so its checks run across both x
    and z. The checks show through the quad, darkened by half and tinted.
*/
void glassScene(v3d::moya::RenderContext & rc) {
    rc.imageResolution({64, 48}, 1.0F);
    pixelCentres(rc);
    rc.clipping({.hither = 1.0F, .yon = 100.0F});
    rc.searchpath("data:&");
    rc.prepareWorld();

    typedef v3d::render::offline::rib::Declaration Declaration;
    v3d::render::offline::rib::ParameterList ambient;
    put(&ambient, "intensity", Declaration::Type::FLOAT, { 0.3F });
    rc.lightSource({.shader = "ambientlight", .handle = "0"}, ambient);
    v3d::render::offline::rib::ParameterList ahead;
    put(&ahead, "intensity", Declaration::Type::FLOAT, { 0.8F });
    put(&ahead, "to", Declaration::Type::POINT, { 0.0F, 0.0F, 1.0F });
    rc.lightSource({.shader = "distantlight", .handle = "1"}, ahead);

    rc.attributeBegin();
    rc.color(glm::vec3(0.9F, 0.85F, 0.7F));
    v3d::render::offline::rib::ParameterList checks;
    put(&checks, "size", Declaration::Type::FLOAT, { 0.4F });
    rc.surface("checked", checks);
    quadAt(rc, {glm::vec3(-1.4F, -1.1F, 4.0F), glm::vec3(-1.4F, 1.1F, 8.0F), glm::vec3(1.4F, 1.1F, 8.0F),
        glm::vec3(1.4F, -1.1F, 4.0F)});
    rc.attributeEnd();

    rc.attributeBegin();
    rc.color(glm::vec3(0.2F, 0.5F, 0.9F));
    rc.opacity(glm::vec3(0.5F));
    rc.surface("constant", v3d::render::offline::rib::ParameterList());
    quadAt(rc, {glm::vec3(-0.6F, -0.5F, 3.0F), glm::vec3(-0.6F, 0.5F, 3.0F), glm::vec3(0.6F, 0.5F, 3.0F),
        glm::vec3(0.6F, -0.5F, 3.0F)});
    rc.attributeEnd();
}

/**
 * A translucent surface under the reyes hider. Each sample keeps the quad and the plane behind
 * it, and composites them front to back once the buckets are done.
 **/
BOOST_AUTO_TEST_CASE(moya_glass_reference_test) {
    v3d::moya::RenderContext rc;
    glassScene(rc);
    rc.render();

    check(rc.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS), GLASS, GLASS_RENDERED);
}

/**
 * The same translucent quad said in RIB, against the same committed picture.
 **/
BOOST_AUTO_TEST_CASE(moya_glass_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(GLASS_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS), GLASS, GLASS_RIB_RENDERED);
}

/*
    The same scene as data/reference-displaced.rib, and as data/raytrace-displaced.rib under the
    ray hider. A plane is rippled three times across x by the lift displacement shader, toward
    the camera, and lit from the left. The plane's normal points away from the camera, so the
    amount is negative.
*/
void displacedScene(v3d::moya::RenderContext & rc, const char* hider) {
    rc.hider(hider);
    rc.imageResolution({64, 48}, 1.0F);
    pixelCentres(rc);
    rc.clipping({.hither = 1.0F, .yon = 100.0F});
    rc.searchpath("data:&");
    rc.prepareWorld();

    typedef v3d::render::offline::rib::Declaration Declaration;
    v3d::render::offline::rib::ParameterList ambient;
    put(&ambient, "intensity", Declaration::Type::FLOAT, { 0.1F });
    rc.lightSource({.shader = "ambientlight", .handle = "0"}, ambient);
    v3d::render::offline::rib::ParameterList side;
    put(&side, "intensity", Declaration::Type::FLOAT, { 1.0F });
    put(&side, "to", Declaration::Type::POINT, { 0.8F, 0.0F, 1.0F });
    rc.lightSource({.shader = "distantlight", .handle = "1"}, side);

    rc.color(glm::vec3(0.8F, 0.8F, 0.85F));
    rc.surface("matte", v3d::render::offline::rib::ParameterList());
    v3d::render::offline::rib::ParameterList bound;
    put(&bound, "sphere", Declaration::Type::FLOAT, { 0.16F });
    rc.attribute("displacementbound", bound);
    v3d::render::offline::rib::ParameterList ripples;
    put(&ripples, "amount", Declaration::Type::FLOAT, { -0.08F });
    put(&ripples, "wave", Declaration::Type::FLOAT, { 0.08F });
    put(&ripples, "frequency", Declaration::Type::FLOAT, { 7.853982F });
    rc.displacement("lift", ripples);
    quadAt(rc, {glm::vec3(-1.2F, -0.9F, 5.0F), glm::vec3(1.2F, -0.9F, 5.0F), glm::vec3(1.2F, 0.9F, 5.0F),
        glm::vec3(-1.2F, 0.9F, 5.0F)});
}

/**
 * A displaced surface under the reyes hider. Each grid is moved by the displacement shader
 * before the surface shader runs, so matte shades the moved normals. The ripples show as light
 * and shade across a plane that is flat in the scene.
 **/
BOOST_AUTO_TEST_CASE(moya_displaced_reference_test) {
    v3d::moya::RenderContext rc;
    displacedScene(rc, "hidden");
    rc.render();

    check(rc.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS), DISPLACED, DISPLACED_RENDERED);
}

/**
 * The same displaced plane said in RIB, against the same committed picture.
 **/
BOOST_AUTO_TEST_CASE(moya_displaced_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(DISPLACED_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS),
        DISPLACED, DISPLACED_RIB_RENDERED);
}

/**
 * The displaced plane under the ray hider. The plane is diced and displaced into the traced
 * scene, so the rays meet the rippled surface and its silhouette. It is not compared with
 * reference-displaced.png, because the hiders sample differently.
 **/
BOOST_AUTO_TEST_CASE(moya_traced_displaced_reference_test) {
    v3d::moya::RenderContext rc;
    displacedScene(rc, "raytrace");
    rc.render();

    check(rc.framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS), TRACED_DISPLACED, TRACED_DISPLACED_RENDERED);
}

/**
 * The same traced displaced plane said in RIB, against the same committed picture.
 **/
BOOST_AUTO_TEST_CASE(moya_traced_displaced_reference_from_rib_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(TRACED_DISPLACED_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().framebuffer()->planes()->image(v3d::moya::FrameBuffer::CHANNELS),
        TRACED_DISPLACED, TRACED_DISPLACED_RIB_RENDERED);
}

};  // namespace

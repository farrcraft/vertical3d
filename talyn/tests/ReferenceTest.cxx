/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Compare.h>
#include <api/image/Factory.h>
#include <api/render/offline/rib/Reader.h>
#include <api/render/offline/sl/ShaderLibrary.h>
#include <talyn/libtalyn/RIBHandler.h>
#include <talyn/libtalyn/RenderContext.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/operations.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

namespace {

const char* REFERENCE = "data/reference-triangle.png";
const char* RENDERED = "data_out/reference-triangle.png";
const char* RIB_SCENE = "data/reference-triangle.rib";
const char* RIB_RENDERED = "data_out/reference-triangle-rib.png";

const char* SHADED = "data/reference-shaded.png";
const char* SHADED_RENDERED = "data_out/reference-shaded.png";
const char* SHADED_SCENE = "data/reference-shaded.rib";
const char* SHADED_RIB_RENDERED = "data_out/reference-shaded-rib.png";

const char* SAMPLED = "data/reference-sampled.png";
const char* SAMPLED_RENDERED = "data_out/reference-sampled.png";
const char* SAMPLED_SCENE = "data/reference-sampled.rib";
const char* SAMPLED_RIB_RENDERED = "data_out/reference-sampled-rib.png";

const char* FOCUS = "data/reference-focus.png";
const char* FOCUS_RENDERED = "data_out/reference-focus.png";
const char* FOCUS_SCENE = "data/reference-focus.rib";
const char* FOCUS_RIB_RENDERED = "data_out/reference-focus-rib.png";

const char* MOTION = "data/reference-motion.png";
const char* MOTION_RENDERED = "data_out/reference-motion.png";
const char* MOTION_SCENE = "data/reference-motion.rib";
const char* MOTION_RIB_RENDERED = "data_out/reference-motion-rib.png";

const char* TRACE = "data/reference-trace.png";
const char* TRACE_RENDERED = "data_out/reference-trace.png";
const char* TRACE_SCENE = "data/reference-trace.rib";
const char* TRACE_RIB_RENDERED = "data_out/reference-trace-rib.png";

/**
 * One sample at each pixel centre under a one pixel box, which the film gives back exactly.
 * The references drawn this way pin the hider and the shading; reference-sampled pins the
 * sampling.
 **/
void pixelCentres(v3d::talyn::RenderContext & rc) {
    rc.sampling().samples = glm::uvec2(1, 1);
    rc.sampling().filter = v3d::render::offline::Filter::Box;
    rc.sampling().width = glm::vec2(1.0f, 1.0f);
}

/*
    Regenerating this reference is expected whenever shading or sampling changes the
    picture on purpose. What it buys is that a change which was not meant to alter the
    picture says so.
*/
boost::shared_ptr<v3d::image::Image> render() {
    v3d::talyn::RenderContext rc;
    rc.format(64, 48);
    pixelCentres(rc);

    v3d::type::camera::Profile & profile = rc.scene().camera().profile();
    profile.orthographic(true);
    // the frame is 4:3, so the pixel has to be, or the picture is stretched across it
    profile.pixelAspect(4.0f / 3.0f);
    profile.orthoZoom(1.0f);
    profile.eye(glm::vec3(0.0f, 0.0f, -1.0f));
    profile.clipping(0.001f, 100.0f);

    rc.scene().background(glm::vec3(0.15f, 0.25f, 0.45f));
    // asymmetric about both axes, so a flipped picture is a failing one
    rc.scene().add(v3d::talyn::Triangle(
        glm::vec3(-0.8f, -0.6f, 2.0f),
        glm::vec3(0.8f, -0.6f, 2.0f),
        glm::vec3(0.0f, 0.7f, 2.0f),
        glm::vec3(0.9f, 0.2f, 0.2f)));

    rc.render();
    return rc.framebuffer()->image(3);
}

v3d::render::offline::sl::ShaderLibrary & library() {
    static v3d::render::offline::sl::ShaderLibrary shaders(
        boost::make_shared<v3d::log::Logger>());
    return shaders;
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

v3d::render::offline::sl::Placed shader(const std::string & name,
    v3d::render::offline::sl::ShaderType type,
    const v3d::render::offline::rib::ParameterList & parameters) {
    v3d::render::offline::sl::Placed placed;
    placed.shader = library().instance(name, type, parameters);
    return placed;
}

/**
 * A quad in one z plane, as the two triangles the reader's fan makes of it.
 **/
void quad(v3d::talyn::Scene* scene, float left, float bottom, float right, float top,
    float z, const glm::vec3 & colour, const v3d::render::offline::sl::Placed & surface) {
    const glm::vec3 corners[4] = {
        glm::vec3(left, bottom, z), glm::vec3(right, bottom, z),
        glm::vec3(right, top, z), glm::vec3(left, top, z)
    };
    for (unsigned int i = 1; i + 1 < 4; i++) {
        v3d::talyn::Triangle triangle(corners[0], corners[i], corners[i + 1], colour);
        triangle.surface(surface);
        scene->add(triangle);
    }
}

/*
    The same scene as data/reference-shaded.rib, built through the render context instead
    of read from a file: a matte floor and a plastic panel, three lights of three kinds,
    and a shadow the panel casts across the floor. It is sampled at the RI defaults unless
    the caller says otherwise.
*/
void shadedScene(v3d::talyn::RenderContext & rc) {
    typedef v3d::render::offline::rib::Declaration Declaration;
    rc.format(64, 48);
    v3d::type::camera::Profile & profile = rc.scene().camera().profile();
    profile.orthographic(true);
    profile.pixelAspect(4.0f / 3.0f);
    profile.orthoZoom(1.0f);
    profile.eye(glm::vec3(0.0f, 0.0f, -1.0f));
    profile.clipping(0.001f, 100.0f);

    v3d::render::offline::rib::ParameterList behind;
    put(&behind, "background", Declaration::Type::COLOR, { 0.05f, 0.06f, 0.1f });
    rc.imager(shader("background", v3d::render::offline::sl::ShaderType::IMAGER, behind));

    v3d::render::offline::rib::ParameterList fill;
    put(&fill, "intensity", Declaration::Type::FLOAT, { 0.18f });
    rc.scene().add(shader("ambientlight", v3d::render::offline::sl::ShaderType::LIGHT, fill));

    v3d::render::offline::rib::ParameterList distant;
    put(&distant, "intensity", Declaration::Type::FLOAT, { 0.9f });
    put(&distant, "to", Declaration::Type::POINT, { 0.7f, -0.7f, 1.0f });
    rc.scene().add(shader("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, distant));

    v3d::render::offline::rib::ParameterList lamp;
    put(&lamp, "intensity", Declaration::Type::FLOAT, { 1.2f });
    v3d::render::offline::sl::Placed bulb = shader("pointlight",
        v3d::render::offline::sl::ShaderType::LIGHT, lamp);
    bulb.placement = glm::translate(glm::mat4x4(1.0f), glm::vec3(-0.6f, 0.5f, 1.2f));
    rc.scene().add(bulb);

    const v3d::render::offline::rib::ParameterList none;
    quad(&rc.scene(), -1.15f, -1.2f, 1.15f, 1.2f, 3.0f, glm::vec3(0.8f, 0.75f, 0.6f),
        shader("matte", v3d::render::offline::sl::ShaderType::SURFACE, none));

    v3d::render::offline::rib::ParameterList shiny;
    put(&shiny, "roughness", Declaration::Type::FLOAT, { 0.1f });
    put(&shiny, "Ks", Declaration::Type::FLOAT, { 0.5f });
    quad(&rc.scene(), -0.5f, -0.45f, 0.35f, 0.4f, 2.2f, glm::vec3(0.2f, 0.45f, 0.8f),
        shader("plastic", v3d::render::offline::sl::ShaderType::SURFACE, shiny));
}

/*
    The same scene as data/reference-focus.rib: a red quad on the plane of focus four units
    out and a blue one ten units out, through a perspective camera at the origin. The lens is
    the file's, or a pinhole.
*/
void focusScene(v3d::talyn::RenderContext & rc, bool lens) {
    rc.format(64, 48);
    rc.sampling().samples = glm::uvec2(4, 4);
    if (lens) {
        rc.sampling().fstop = 1.0f;
        rc.sampling().focalLength = 0.5f;
        rc.sampling().focalDistance = 4.0f;
    }
    v3d::type::camera::Profile & profile = rc.scene().camera().profile();
    profile.orthographic(false);
    profile.pixelAspect(4.0f / 3.0f);
    profile.fov(40.0f);
    profile.eye(glm::vec3(0.0f));
    profile.clipping(0.1f, 100.0f);

    const v3d::render::offline::sl::Placed constant = shader("constant",
        v3d::render::offline::sl::ShaderType::SURFACE, v3d::render::offline::rib::ParameterList());
    quad(&rc.scene(), -1.5f, -1.0f, -0.2f, 1.0f, 4.0f, glm::vec3(0.9f, 0.2f, 0.2f), constant);
    quad(&rc.scene(), 0.5f, -2.5f, 4.0f, 2.5f, 10.0f, glm::vec3(0.2f, 0.4f, 0.9f), constant);
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
 * A quad in one z plane under a transformation that may move, as the reader's fan makes it.
 **/
void movingQuad(v3d::talyn::Scene* scene, const glm::vec3 (&corners)[4], const glm::vec3 & colour,
    const v3d::render::offline::sl::Placed & surface, const v3d::render::offline::MovingTransform & placed) {
    for (unsigned int i = 1; i + 1 < 4; i++) {
        const glm::vec3 a(placed.open() * glm::vec4(corners[0], 1.0f));
        const glm::vec3 b(placed.open() * glm::vec4(corners[i], 1.0f));
        const glm::vec3 c(placed.open() * glm::vec4(corners[i + 1], 1.0f));
        v3d::talyn::Triangle triangle(a, b, c, colour);
        triangle.surface(surface);
        scene->add(triangle, placed);
    }
}

/**
 * The orthographic camera of the other references, with the shutter open from 0 to 1.
 **/
void motionCamera(v3d::talyn::RenderContext & rc) {
    rc.format(64, 48);
    rc.sampling().samples = glm::uvec2(4, 4);
    rc.sampling().shutter = glm::vec2(0.0f, 1.0f);
    v3d::type::camera::Profile & profile = rc.scene().camera().profile();
    profile.orthographic(true);
    profile.pixelAspect(4.0f / 3.0f);
    profile.orthoZoom(1.0f);
    profile.eye(glm::vec3(0.0f));
    profile.clipping(0.1f, 100.0f);
}

/*
    The same scene as data/reference-motion.rib: a quad sliding right and a quad turning a
    quarter about its centre, each across the whole shutter.
*/
void motionScene(v3d::talyn::RenderContext & rc) {
    motionCamera(rc);
    const v3d::render::offline::sl::Placed constant = shader("constant",
        v3d::render::offline::sl::ShaderType::SURFACE, v3d::render::offline::rib::ParameterList());

    v3d::render::offline::MovingTransform sliding;
    sliding.begin({ 0.0f, 1.0f });
    sliding.concat(glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f)));
    sliding.concat(glm::translate(glm::mat4x4(1.0f), glm::vec3(0.5f, 0.0f, 0.0f)));
    sliding.end();
    const glm::vec3 slid[4] = {
        glm::vec3(-1.2f, 0.1f, 5.0f), glm::vec3(-0.4f, 0.1f, 5.0f),
        glm::vec3(-0.4f, 0.8f, 5.0f), glm::vec3(-1.2f, 0.8f, 5.0f)
    };
    movingQuad(&rc.scene(), slid, glm::vec3(0.9f, 0.8f, 0.2f), constant, sliding);

    v3d::render::offline::MovingTransform turning(glm::translate(glm::mat4x4(1.0f), glm::vec3(0.6f, -0.4f, 5.0f)));
    turning.begin({ 0.0f, 1.0f });
    turning.concat(glm::rotate(glm::mat4x4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)));
    turning.concat(glm::rotate(glm::mat4x4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
    turning.end();
    const glm::vec3 turned[4] = {
        glm::vec3(-0.35f, -0.15f, 0.0f), glm::vec3(0.35f, -0.15f, 0.0f),
        glm::vec3(0.35f, 0.15f, 0.0f), glm::vec3(-0.35f, 0.15f, 0.0f)
    };
    movingQuad(&rc.scene(), turned, glm::vec3(0.2f, 0.7f, 0.9f), constant, turning);
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
 * A quad through four corners, as the two triangles the reader's fan makes of it.
 **/
void polygon(v3d::talyn::Scene* scene, const glm::vec3 (&corners)[4], const glm::vec3 & colour,
    const v3d::render::offline::sl::Placed & surface) {
    for (unsigned int i = 1; i + 1 < 4; i++) {
        v3d::talyn::Triangle triangle(corners[0], corners[i], corners[i + 1], colour);
        triangle.surface(surface);
        scene->add(triangle);
    }
}

/*
    The same scene as data/reference-trace.rib: a metal sphere and a glass one over a
    checked floor, in front of a wall, through a perspective camera at the origin and
    sampled at the RI defaults.
*/
void traceScene(v3d::talyn::RenderContext & rc) {
    typedef v3d::render::offline::rib::Declaration Declaration;
    typedef v3d::render::offline::sl::ShaderType ShaderType;
    rc.format(64, 48);
    v3d::type::camera::Profile & profile = rc.scene().camera().profile();
    profile.orthographic(false);
    profile.pixelAspect(4.0f / 3.0f);
    profile.fov(40.0f);
    profile.eye(glm::vec3(0.0f));
    profile.clipping(0.1f, 100.0f);
    // the floor's shader is a fixture beside the scene rather than one of the standard ones
    library().searchpath("data:&");

    v3d::render::offline::rib::ParameterList fill;
    put(&fill, "intensity", Declaration::Type::FLOAT, { 0.2f });
    rc.scene().add(shader("ambientlight", ShaderType::LIGHT, fill));
    v3d::render::offline::rib::ParameterList distant;
    put(&distant, "intensity", Declaration::Type::FLOAT, { 0.9f });
    put(&distant, "to", Declaration::Type::POINT, { 0.4f, -1.0f, 0.6f });
    rc.scene().add(shader("distantlight", ShaderType::LIGHT, distant));

    v3d::render::offline::rib::ParameterList checks;
    put(&checks, "size", Declaration::Type::FLOAT, { 0.5f });
    const glm::vec3 floor[4] = {
        glm::vec3(-4.0f, -1.0f, 2.0f), glm::vec3(4.0f, -1.0f, 2.0f),
        glm::vec3(4.0f, -1.0f, 12.0f), glm::vec3(-4.0f, -1.0f, 12.0f)
    };
    polygon(&rc.scene(), floor, glm::vec3(0.85f, 0.8f, 0.7f), shader("checked", ShaderType::SURFACE, checks));
    const glm::vec3 wall[4] = {
        glm::vec3(-4.0f, -1.0f, 12.0f), glm::vec3(4.0f, -1.0f, 12.0f),
        glm::vec3(4.0f, 4.0f, 12.0f), glm::vec3(-4.0f, 4.0f, 12.0f)
    };
    polygon(&rc.scene(), wall, glm::vec3(0.35f, 0.5f, 0.75f),
        shader("matte", ShaderType::SURFACE, v3d::render::offline::rib::ParameterList()));

    v3d::render::offline::rib::ParameterList mirror;
    put(&mirror, "Ka", Declaration::Type::FLOAT, { 0.1f });
    put(&mirror, "Ks", Declaration::Type::FLOAT, { 0.6f });
    put(&mirror, "Kr", Declaration::Type::FLOAT, { 0.8f });
    const glm::mat4x4 left = glm::translate(glm::mat4x4(1.0f), glm::vec3(-1.1f, -0.28f, 6.0f));
    v3d::talyn::Sphere metal(0.7f, -0.7f, 0.7f, 360.0f, left, glm::vec3(0.9f, 0.85f, 0.7f));
    metal.surface(shader("shinymetal", ShaderType::SURFACE, mirror));
    rc.scene().add(metal, v3d::render::offline::MovingTransform(left));

    const glm::mat4x4 right = glm::translate(glm::mat4x4(1.0f), glm::vec3(1.1f, -0.28f, 5.5f));
    v3d::talyn::Sphere glass(0.7f, -0.7f, 0.7f, 360.0f, right, glm::vec3(1.0f));
    glass.surface(shader("glass", ShaderType::SURFACE, v3d::render::offline::rib::ParameterList()));
    glass.opacity(glm::vec3(0.3f));
    rc.scene().add(glass, v3d::render::offline::MovingTransform(right));
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

};  // namespace

/**
 * A rendered image against a committed one. Neither renderer touches a window, a device or a
 * swapchain, so unlike everything below the recorder in api/render this runs in CI.
 **/
BOOST_AUTO_TEST_CASE(talyn_reference_test) {
    boost::shared_ptr<v3d::image::Image> rendered = render();

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
 * neither of them is the reference. The background is a backdrop polygon rather than a scene
 * colour, because RiImager is not implemented - so the file says with geometry what the code
 * built scene says with Scene::background().
 **/
BOOST_AUTO_TEST_CASE(talyn_reference_from_rib_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(RIB_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");
    BOOST_REQUIRE_EQUAL(handler.error(), "");

    rc->render();
    boost::shared_ptr<v3d::image::Image> rendered = rc->framebuffer()->image(3);

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
 * A lit scene, read from a file and rendered.
 *
 * This is the first talyn picture with any shading in it: a surface shader, a light, and a
 * value that is neither the geometry's colour nor black. It asserts the number rather than
 * a committed image, because what a lit picture should look like is step 12's question and
 * what the wiring does is this step's.
 **/
BOOST_AUTO_TEST_CASE(talyn_renders_a_lit_scene_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read("data/lit-quad.rib", &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");
    BOOST_REQUIRE_EQUAL(handler.error(), "");

    rc->render();
    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = rc->framebuffer();

    /*
        The light is forty five degrees off the surface, so a white quad under it comes out
        at the cosine of that - which is a value the geometry's own colour could not have
        produced and neither could no shading at all.
    */
    BOOST_CHECK_CLOSE(planes->value(0, 32, 24), 0.70710678f, 0.5f);
    BOOST_CHECK_CLOSE(planes->value(2, 32, 24), 0.70710678f, 0.5f);
    // and nothing outside the quad, which is the background
    BOOST_CHECK_SMALL(planes->value(0, 2, 2), 0.0001f);
}

/**
 * The first talyn picture with shading in it: a matte floor and a plastic panel, three
 * lights of three kinds, and the panel's shadow falling across the floor.
 *
 * A shader is not tested by a picture - the language's own cases are in
 * v3dtest_render_offline. What this catches is the wiring: an ambient() that reached no
 * light, a shadow ray that started on the surface it left, an imager that ran over the
 * wrong plane. None of those is visible to a unit case and all of them are visible here.
 **/
BOOST_AUTO_TEST_CASE(talyn_shaded_reference_test) {
    v3d::talyn::RenderContext rc;
    shadedScene(rc);
    pixelCentres(rc);
    rc.render();

    check(rc.framebuffer()->image(3), SHADED, SHADED_RENDERED);
}

/**
 * The same shaded scene said in RIB, against the same committed picture.
 **/
BOOST_AUTO_TEST_CASE(talyn_shaded_reference_from_rib_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(SHADED_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");
    BOOST_REQUIRE_EQUAL(handler.error(), "");

    rc->render();
    check(rc->framebuffer()->image(3), SHADED, SHADED_RIB_RENDERED);
}

/**
 * The shaded scene at the RI defaults, two by two samples under a gaussian two pixels wide,
 * per ADR-0076. Its edges are antialiased, and it is the same on every run because every
 * pixel's samples are seeded by where the pixel is.
 **/
BOOST_AUTO_TEST_CASE(talyn_sampled_reference_test) {
    v3d::talyn::RenderContext rc;
    shadedScene(rc);
    rc.render();

    check(rc.framebuffer()->image(3), SAMPLED, SAMPLED_RENDERED);
}

/**
 * The sampled scene said in RIB, which names no sampling and so gets the defaults.
 **/
BOOST_AUTO_TEST_CASE(talyn_sampled_reference_from_rib_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(SAMPLED_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");
    BOOST_REQUIRE_EQUAL(handler.error(), "");

    rc->render();
    check(rc->framebuffer()->image(3), SAMPLED, SAMPLED_RIB_RENDERED);
}

/**
 * Two renders of the sampled scene are equal byte for byte, which is what lets a reference
 * survive sampling at all.
 **/
BOOST_AUTO_TEST_CASE(talyn_sampled_render_is_repeatable_test) {
    v3d::talyn::RenderContext first;
    shadedScene(first);
    first.render();
    v3d::talyn::RenderContext second;
    shadedScene(second);
    second.render();

    const v3d::image::Difference difference = v3d::image::compare(
        *first.framebuffer()->image(3), *second.framebuffer()->image(3), 0);
    BOOST_CHECK_MESSAGE(difference.match, difference.description());
}

/**
 * Two quads at two depths through a lens focused on the nearer one.
 **/
BOOST_AUTO_TEST_CASE(talyn_focus_reference_test) {
    v3d::talyn::RenderContext rc;
    focusScene(rc, true);
    rc.render();

    check(rc.framebuffer()->image(3), FOCUS, FOCUS_RENDERED);
}

BOOST_AUTO_TEST_CASE(talyn_focus_reference_from_rib_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(FOCUS_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");
    BOOST_REQUIRE_EQUAL(handler.error(), "");

    rc->render();
    check(rc->framebuffer()->image(3), FOCUS, FOCUS_RIB_RENDERED);
}

/**
 * The quad on the plane of focus is as sharp through the lens as through a pinhole: every
 * ray aimed at a point on that plane still reaches it. The block is the columns the blue
 * quad's blur does not reach.
 **/
BOOST_AUTO_TEST_CASE(talyn_in_focus_is_sharp_test) {
    v3d::talyn::RenderContext pinhole;
    focusScene(pinhole, false);
    pinhole.render();
    v3d::talyn::RenderContext lens;
    focusScene(lens, true);
    lens.render();

    BOOST_CHECK_SMALL(largest(*pinhole.framebuffer(), *lens.framebuffer(), 0, 30), 1.0f / 255.0f);
}

/**
 * The quad off the plane of focus spreads its edge over its circle of confusion. Under a one
 * pixel box, a pinhole leaves the edge in a pixel or two; the lens, a quarter of a unit across
 * and focused four units out, blurs a point ten units out over 2 * 0.25 * (10 - 4) / 10 of a
 * unit there, which is about five pixels at this field of view.
 **/
BOOST_AUTO_TEST_CASE(talyn_out_of_focus_spreads_test) {
    v3d::talyn::RenderContext pinhole;
    focusScene(pinhole, false);
    pinhole.sampling().filter = v3d::render::offline::Filter::Box;
    pinhole.sampling().width = glm::vec2(1.0f);
    pinhole.render();
    v3d::talyn::RenderContext lens;
    focusScene(lens, true);
    lens.sampling().filter = v3d::render::offline::Filter::Box;
    lens.sampling().width = glm::vec2(1.0f);
    lens.render();

    const unsigned int sharp = partial(*pinhole.framebuffer(), 3, 24, 30, 45);
    const unsigned int blurred = partial(*lens.framebuffer(), 3, 24, 30, 45);
    BOOST_CHECK_LE(sharp, 2u);
    BOOST_CHECK_GE(blurred, 4u);
    BOOST_CHECK_LE(blurred, 7u);
}

/**
 * A quad sliding and a quad turning while the shutter is open.
 **/
BOOST_AUTO_TEST_CASE(talyn_motion_reference_test) {
    v3d::talyn::RenderContext rc;
    motionScene(rc);
    rc.render();

    check(rc.framebuffer()->image(3), MOTION, MOTION_RENDERED);
}

BOOST_AUTO_TEST_CASE(talyn_motion_reference_from_rib_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(MOTION_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");
    BOOST_REQUIRE_EQUAL(handler.error(), "");

    rc->render();
    check(rc->framebuffer()->image(3), MOTION, MOTION_RIB_RENDERED);
}

/**
 * A quad translated across the shutter spreads over the distance it moved, its coverage
 * rising and falling linearly along it.
 **/
BOOST_AUTO_TEST_CASE(talyn_motion_spreads_linearly_test) {
    v3d::talyn::RenderContext rc;
    motionCamera(rc);
    rc.sampling().samples = glm::uvec2(8, 8);
    rc.sampling().filter = v3d::render::offline::Filter::Box;
    rc.sampling().width = glm::vec2(1.0f);

    v3d::render::offline::MovingTransform sliding;
    sliding.begin({ 0.0f, 1.0f });
    sliding.concat(glm::mat4x4(1.0f));
    sliding.concat(glm::translate(glm::mat4x4(1.0f), glm::vec3(0.5f, 0.0f, 0.0f)));
    sliding.end();
    const glm::vec3 corners[4] = {
        glm::vec3(-1.0f, -0.5f, 5.0f), glm::vec3(0.0f, -0.5f, 5.0f),
        glm::vec3(0.0f, 0.5f, 5.0f), glm::vec3(-1.0f, 0.5f, 5.0f)
    };
    movingQuad(&rc.scene(), corners, glm::vec3(1.0f), v3d::render::offline::sl::Placed(), sliding);
    rc.render();

    checkRamp(*rc.framebuffer(), 3, 24);
}

/**
 * A metal sphere and a glass one over a checked floor: reflection, refraction, the fresnel
 * split between them, spheres, and a shadow through an occluder that is not opaque.
 **/
BOOST_AUTO_TEST_CASE(talyn_trace_reference_test) {
    v3d::talyn::RenderContext rc;
    traceScene(rc);
    rc.render();

    check(rc.framebuffer()->image(3), TRACE, TRACE_RENDERED);
}

BOOST_AUTO_TEST_CASE(talyn_trace_reference_from_rib_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBHandler handler(rc);
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(TRACE_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");
    BOOST_REQUIRE_EQUAL(handler.error(), "");

    rc->render();
    check(rc->framebuffer()->image(3), TRACE, TRACE_RIB_RENDERED);
}

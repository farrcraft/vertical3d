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

/*
    Regenerating this reference is expected whenever shading or sampling changes the
    picture on purpose. What it buys is that a change which was not meant to alter the
    picture says so.
*/
boost::shared_ptr<v3d::image::Image> render() {
    v3d::talyn::RenderContext rc;
    rc.format(64, 48);

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
    and a shadow the panel casts across the floor.
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

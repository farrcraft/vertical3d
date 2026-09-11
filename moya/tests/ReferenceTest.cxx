/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Compare.h>
#include <api/image/Factory.h>
#include <api/render/offline/rib/Reader.h>
#include <moya/libmoya/RIBHandler.h>
#include <moya/libmoya/RenderContext.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/operations.hpp>

#include <glm/glm.hpp>

namespace {

const char* REFERENCE = "data/reference-polygon.png";
const char* RENDERED = "data_out/reference-polygon.png";
const char* RIB_SCENE = "data/reference-polygon.rib";
const char* RIB_RENDERED = "data_out/reference-polygon-rib.png";

const char* SHADED = "data/reference-shaded.png";
const char* SHADED_RENDERED = "data_out/reference-shaded.png";
const char* SHADED_SCENE = "data/reference-shaded.rib";
const char* SHADED_RIB_RENDERED = "data_out/reference-shaded-rib.png";

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
    distant light and a point light close enough for its falloff to show.
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
 * A rendered image against a committed one. Neither renderer touches a window, a device or a
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

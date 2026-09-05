/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/operations.hpp>

#include <glm/glm.hpp>

#include "../libmoya/RenderContext.h"

#include "../../api/image/Compare.h"
#include "../../api/image/Factory.h"

namespace {

    const char* REFERENCE = "data/reference-polygon.png";
    const char* RENDERED = "data_out/reference-polygon.png";

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

        boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
        polygon->addVertex(vertex(-0.7f, -0.2f, 5.0f));
        polygon->addVertex(vertex(0.3f, -0.2f, 5.0f));
        polygon->addVertex(vertex(0.3f, 0.8f, 5.0f));
        polygon->addVertex(vertex(-0.7f, 0.8f, 5.0f));
        rc.addPolygon(polygon);
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

    // the quad covers raster x over [9.6, 41.6] and y over [4.8, 28.8]
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

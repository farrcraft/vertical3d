/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/operations.hpp>

#include "../libtalyn/RenderContext.h"
#include "../libtalyn/RIBHandler.h"

#include "../../api/render/offline/RIBReader.h"

#include "../../api/image/Compare.h"
#include "../../api/image/Factory.h"

namespace {

const char* REFERENCE = "data/reference-triangle.png";
const char* RENDERED = "data_out/reference-triangle.png";
const char* RIB_SCENE = "data/reference-triangle.rib";
const char* RIB_RENDERED = "data_out/reference-triangle-rib.png";

/*
    Regenerating this reference is expected whenever shading or sampling changes the
    picture on purpose. What it buys is that a change which was not meant to alter the
    picture says so.
*/
boost::shared_ptr<v3d::image::Image> render() {
    v3d::talyn::RenderContext rc;
    rc.format(64, 48);

    v3d::type::CameraProfile & profile = rc.scene().camera().profile();
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
    v3d::render::offline::RIBReader reader(boost::make_shared<v3d::log::Logger>());

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

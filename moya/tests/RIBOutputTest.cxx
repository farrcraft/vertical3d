/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/render/offline/FrameBuffer.h>
#include <moya/libmoya/Planes.h>
#include <moya/libmoya/RIBHandler.h>
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

#include "Handling.h"

namespace v3d::moya::handling {
namespace {

/**
 * A picture the image factory cannot write is reported through the context's own logger.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_a_failed_write_is_logged_test) {
    v3d::moya::RIBHandler handler;
    const Heard heard(handler.context().logger()->get());

    BOOST_REQUIRE(read(
        "Format 8 6 1\n"
        "Display \"picture.unknown\" \"file\" \"rgb\"\n"
        "WorldBegin\n"
        "WorldEnd\n", &handler));

    BOOST_TEST(heard.said("picture.unknown"));
}

/**
 * The renderer reads no display parameter, so each one a scene gives is logged as not used.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_a_display_parameter_is_logged_test) {
    v3d::moya::RIBHandler handler;
    const Heard heard(handler.context().logger()->get());

    BOOST_REQUIRE(read(
        "Declare \"origin\" \"uniform integer[2]\"\n"
        "Display \"picture.tif\" \"framebuffer\" \"rgb\" \"origin\" [4 2]\n", &handler));

    BOOST_TEST(heard.said("origin"));
}

/**
 * A red quad that covers an 8 by 6 picture, under the crop window's left half, with the hider
 * given. RI renders the columns from ceil(8 * 0) to ceil(8 * 0.5) - 1, so columns 0 to 3.
 **/
boost::shared_ptr<v3d::render::offline::FrameBuffer> cropped(const std::string & hider) {
    v3d::moya::RIBHandler handler;
    BOOST_REQUIRE(read(
        "Format 8 6 1\n"
        "PixelSamples 1 1\n"
        "PixelFilter \"box\" 1 1\n"
        "CropWindow 0 0.5 0 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n" + hider +
        "WorldBegin\n"
        "Color [1 0 0]\n"
        "Polygon \"P\" [-5 -5 5  5 -5 5  5 5 5  -5 5 5]\n"
        "WorldEnd\n", &handler));
    return handler.context().picture();
}

void checkCropped(const v3d::render::offline::FrameBuffer & planes) {
    for (unsigned int row = 0; row < planes.height(); row++) {
        for (unsigned int column = 0; column < planes.width(); column++) {
            const bool inside = column < 4;
            BOOST_TEST_CONTEXT("pixel " << column << ", " << row) {
                BOOST_TEST((planes.value(v3d::moya::Planes::RED, column, row) > 0.5F) == inside);
                BOOST_TEST((planes.value(v3d::moya::Planes::COVERAGE, column, row) > 0.5F) == inside);
            }
        }
    }
}

/**
 * The reyes hider leaves every pixel outside the crop window empty.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_crop_window_under_reyes_test) {
    checkCropped(*cropped(""));
}

/**
 * The ray hider casts no ray outside the crop window, and leaves those pixels empty.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_crop_window_under_the_ray_hider_test) {
    checkCropped(*cropped("Hider \"raytrace\"\n"));
}

/**
 * A red square over the whole of a 64 by 48 picture, under either hider.
 **/
std::string covered(const char* hider) {
    return std::string("Format 64 48 1\n") +
        "Hider \"" + hider + "\"\n"
        "Projection \"orthographic\"\n"
        "ScreenWindow -1 1 -1 1\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Color [1 0 0]\n"
        "Polygon \"P\" [-2 -2 5  2 -2 5  2 2 5  -2 2 5]\n"
        "WorldEnd\n";
}

/**
 * The progress callback is told how far each hider has got, rising to the whole frame.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_progress_test) {
    for (const char* hider : { "hidden", "raytrace" }) {
        BOOST_TEST_CONTEXT(hider) {
            v3d::moya::RIBHandler handler;
            std::vector<float> told;
            handler.context().progress([&told](float done) {
                told.push_back(done);
                return true;
            });
            BOOST_REQUIRE(read(covered(hider), &handler));
            BOOST_REQUIRE(!told.empty());
            BOOST_CHECK(std::is_sorted(told.begin(), told.end()));
            BOOST_CHECK_CLOSE(told.back(), 1.0F, 0.001F);
            BOOST_CHECK(!handler.context().stopped());
            BOOST_CHECK_EQUAL(handler.context().picture()->value(v3d::moya::Planes::RED, 60, 44), 1.0F);
        }
    }
}

/**
 * A callback that answers false stops the render where it is, and the rest of the picture is
 * never drawn.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_stop_test) {
    for (const char* hider : { "hidden", "raytrace" }) {
        BOOST_TEST_CONTEXT(hider) {
            v3d::moya::RIBHandler handler;
            unsigned int told = 0;
            handler.context().progress([&told](float /*done*/) {
                told++;
                return false;
            });
            BOOST_REQUIRE(read(covered(hider), &handler));
            BOOST_CHECK_EQUAL(told, 1U);
            BOOST_CHECK(handler.context().stopped());
            const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().picture();
            BOOST_CHECK_EQUAL(planes->value(v3d::moya::Planes::RED, 60, 44), 0.0F);
        }
    }
}

};  // namespace
};  // namespace v3d::moya::handling

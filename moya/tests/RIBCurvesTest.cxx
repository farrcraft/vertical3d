/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/FrameBuffer.h>
#include <moya/libmoya/Planes.h>
#include <moya/libmoya/RIBHandler.h>

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include "Handling.h"

namespace v3d::moya::handling {
namespace {

/**
 * A 64 by 48 orthographic frame at one sample a pixel, with the hider named, the world holding
 * the requests given, in red under the constant surface. A unit is 24 pixels, x = 0 is column
 * 32 and y = 0 is row 24, with y up.
 **/
boost::shared_ptr<v3d::render::offline::FrameBuffer> render(const std::string & hider, const std::string & world) {
    v3d::moya::RIBHandler handler;
    BOOST_REQUIRE(read(
        "Hider \"" + hider + "\"\n"
        "Format 64 48 1\n"
        "PixelSamples 1 1\n"
        "PixelFilter \"box\" 1 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Color [1 0 0]\n" + world +
        "WorldEnd\n", &handler));
    return handler.context().picture();
}

float red(const boost::shared_ptr<v3d::render::offline::FrameBuffer> & planes, unsigned int column, unsigned int row) {
    return planes->value(v3d::moya::Planes::RED, column, row);
}

BOOST_AUTO_TEST_SUITE(moya_curves)

/**
 * A straight curve across the frame is a ribbon of its width under either hider: rows within
 * half its width of it are covered, and rows further off and columns past its ends are not.
 **/
BOOST_AUTO_TEST_CASE(a_linear_curve_is_a_ribbon_of_its_width) {
    for (const char* hider : { "hidden", "raytrace" }) {
        const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes =
            render(hider, "Curves \"linear\" [2] \"nonperiodic\" \"P\" [-1 0 5  1 0 5] \"constantwidth\" [0.2]\n");
        // the ribbon is 4.8 pixels tall about row 24
        BOOST_CHECK_MESSAGE(red(planes, 32, 23) > 0.5F, hider);
        BOOST_CHECK_MESSAGE(red(planes, 32, 24) > 0.5F, hider);
        BOOST_CHECK_MESSAGE(red(planes, 10, 24) > 0.5F, hider);
        BOOST_CHECK_MESSAGE(red(planes, 32, 28) < 0.5F, hider);
        BOOST_CHECK_MESSAGE(red(planes, 32, 19) < 0.5F, hider);
        // x = 1 is column 56
        BOOST_CHECK_MESSAGE(red(planes, 60, 24) < 0.5F, hider);
    }
}

/**
 * A cubic curve follows its basis. A Bezier arch from (-1, 0) to (1, 0) with handles at height
 * 1 rises to 0.75 at its middle, which is row 6, and does not pass through the middle of its
 * chord.
 **/
BOOST_AUTO_TEST_CASE(a_cubic_curve_follows_its_basis) {
    for (const char* hider : { "hidden", "raytrace" }) {
        const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = render(hider,
            "Basis \"bezier\" 3 \"bezier\" 3\n"
            "Curves \"cubic\" [4] \"nonperiodic\" \"P\" [-1 0 5  -1 1 5  1 1 5  1 0 5] \"constantwidth\" [0.2]\n");
        BOOST_CHECK_MESSAGE(red(planes, 32, 6) > 0.5F, hider);
        BOOST_CHECK_MESSAGE(red(planes, 32, 24) < 0.5F, hider);
        BOOST_CHECK_MESSAGE(red(planes, 32, 12) < 0.5F, hider);
    }

    // the same points under the B-spline basis make a curve that stays inside them, lower
    const boost::shared_ptr<v3d::render::offline::FrameBuffer> spline = render("hidden",
        "Basis \"b-spline\" 1 \"b-spline\" 1\n"
        "Curves \"cubic\" [4] \"nonperiodic\" \"P\" [-1 0 5  -1 1 5  1 1 5  1 0 5] \"constantwidth\" [0.1]\n");
    BOOST_CHECK(red(spline, 32, 6) < 0.5F);
    BOOST_CHECK(red(spline, 32, 3) < 0.5F);
}

/**
 * A varying width sets each end of each segment: a ribbon that narrows to nothing covers less
 * of the frame toward its narrow end.
 **/
BOOST_AUTO_TEST_CASE(a_varying_width_tapers_the_ribbon) {
    for (const char* hider : { "hidden", "raytrace" }) {
        const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes =
            render(hider, "Curves \"linear\" [2] \"nonperiodic\" \"P\" [-1 0 5  1 0 5] \"width\" [0.5 0.0]\n");
        // 0.5 wide at x = -1, column 8, and nearly nothing at x = 0.9, column 54
        BOOST_CHECK_MESSAGE(red(planes, 10, 20) > 0.5F, hider);
        BOOST_CHECK_MESSAGE(red(planes, 54, 21) < 0.5F, hider);
    }
}

/**
 * A curve given no width is RI's one unit wide, which is 24 pixels and wider than a grid of the
 * default 256 micropolygons. Halving the segment would never narrow it, so it is diced whole
 * across its width.
 **/
BOOST_AUTO_TEST_CASE(a_ribbon_wider_than_a_grid_is_diced) {
    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes =
        render("hidden", "Curves \"linear\" [2] \"nonperiodic\" \"P\" [-1 0 5  1 0 5]\n");
    // half a unit either side of row 24 is rows 12 to 36
    BOOST_CHECK(red(planes, 32, 14) > 0.5F);
    BOOST_CHECK(red(planes, 32, 34) > 0.5F);
    BOOST_CHECK(red(planes, 32, 40) < 0.5F);
}

/**
 * A curve whose counts reach past its points draws nothing past them, and says so.
 **/
BOOST_AUTO_TEST_CASE(counts_past_the_points_draw_nothing) {
    v3d::moya::RIBHandler handler;
    const Heard heard(handler.context().logger()->get());
    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Curves \"linear\" [2 5] \"nonperiodic\" \"P\" [-1 0 5  1 0 5  0 1 5]\n"
        "WorldEnd\n", &handler));
    BOOST_TEST(heard.said("reach past"));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
};  // namespace v3d::moya::handling

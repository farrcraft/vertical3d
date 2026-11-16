/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/render/offline/trace/Scene.h>
#include <moya/libmoya/ri/RIBHandler.h>
#include <spdlog/sinks/ringbuffer_sink.h>

#include <algorithm>
#include <cstddef>
#include <iostream>
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
 * Option "limits" is how a scene names the bucket and grid sizes. The standard's example file
 * opens with it.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_limits_test) {
    v3d::moya::ri::RIBHandler handler;

    BOOST_REQUIRE(read(
        "Option \"limits\" \"bucketsize\" [6 6]\n"
        "Option \"limits\" \"gridsize\" [18]\n", &handler));

    BOOST_CHECK_EQUAL(handler.context().bucketWidth(), 6U);
    BOOST_CHECK_EQUAL(handler.context().bucketHeight(), 6U);
    BOOST_CHECK_EQUAL(handler.context().gridSize(), 18U);
}

/**
 * A size pinned before the scene is read survives a scene that names its own with
 * Option "limits". A size that is not pinned is replaced.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_pinned_limits_win_test) {
    v3d::moya::ri::RIBHandler handler;
    handler.context().pinGridSize(64);
    handler.context().pinBucketSize(8, 8);

    BOOST_REQUIRE(read(
        "Option \"limits\" \"bucketsize\" [6 6]\n"
        "Option \"limits\" \"gridsize\" [18]\n", &handler));
    BOOST_CHECK_EQUAL(handler.context().gridSize(), 64U);
    BOOST_CHECK_EQUAL(handler.context().bucketWidth(), 8U);

    // a pin of zero is refused, and pins nothing
    v3d::moya::ri::RIBHandler loose;
    loose.context().pinGridSize(0);
    BOOST_REQUIRE(read("Option \"limits\" \"gridsize\" [18]\n", &loose));
    BOOST_CHECK_EQUAL(loose.context().gridSize(), 18U);
}

/**
 * Option "trace" "maxdepth" sets how deep trace() goes. A depth above sixteen is taken as
 * sixteen. A depth that is negative or not finite is not used, and the depth stays as it was.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_trace_depth_test) {
    v3d::moya::ri::RIBHandler handler;

    BOOST_REQUIRE(read("Option \"trace\" \"maxdepth\" [5]\n", &handler));
    BOOST_CHECK_EQUAL(handler.context().traced().traceDepth(), 5U);

    BOOST_REQUIRE(read("Option \"trace\" \"maxdepth\" [-3]\n", &handler));
    BOOST_CHECK_EQUAL(handler.context().traced().traceDepth(), 5U);

    BOOST_REQUIRE(read("Option \"trace\" \"maxdepth\" [1e30]\n", &handler));
    BOOST_CHECK_EQUAL(handler.context().traced().traceDepth(), 16U);

    BOOST_REQUIRE(read("Option \"trace\" \"maxdepth\" [0]\n", &handler));
    BOOST_CHECK_EQUAL(handler.context().traced().traceDepth(), 0U);
}

/**
 * The driver's --output means that file whatever the scene's Display names.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_output_override_test) {
    v3d::moya::ri::RIBHandler handler;
    handler.output("data_out/override.png");

    BOOST_REQUIRE(read("Display \"scene-says.png\" \"file\" \"rgb\"\n", &handler));

    // nothing is written until WorldEnd, so the case reads the context and not the disk
    BOOST_CHECK_EQUAL(handler.context().displayName(), "data_out/override.png");
}

/**
 * The first handler a process makes prints Pixar's copyright notice, and no later handler
 * prints it again.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_the_notice_is_printed_once_test) {
    const v3d::moya::ri::RIBHandler first;
    const std::ostringstream printed;
    std::streambuf * const console = std::cout.rdbuf(printed.rdbuf());
    {
        const v3d::moya::ri::RIBHandler second;
    }
    std::cout.rdbuf(console);

    BOOST_TEST(printed.str().empty());
}

/**
 * The hiders do not instance objects, so the geometry inside an object's block is left out, as
 * RI leaves it out where it is declared, and the first object is reported. A quad drawn in an
 * object and instanced covers nothing.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_an_objects_geometry_is_left_out_test) {
    v3d::moya::ri::RIBHandler handler;
    const Heard heard(handler.context().logger()->get());
    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "PixelSamples 1 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "ObjectBegin 1\n"
        "Polygon \"P\" [-0.9 -0.9 5  -0.9 0.9 5  0.9 0.9 5  0.9 -0.9 5]\n"
        "ObjectEnd\n"
        "ObjectInstance 1\n"
        "WorldEnd\n", &handler));
    BOOST_TEST(coverage(*handler.context().picture()) == 0U);
    BOOST_TEST(heard.said("do not draw ObjectBegin"));
}

/**
 * Each request the hiders do not draw is reported once, by name.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_geometry_the_hiders_do_not_draw_is_reported_test) {
    v3d::moya::ri::RIBHandler handler;
    const Heard heard(handler.context().logger()->get());
    BOOST_REQUIRE(read(
        "WorldBegin\n"
        "Cone 1 1 360\nCone 1 1 360\nCylinder 1 -1 1 360\nHyperboloid 1 0 -1 1 0 1 360\nParaboloid 1 0 1 360\nDisk 0 1 360\n"
        "Torus 1 0.25 0 360 360\nPoints \"P\" [0 0 5]\n"
        "SubdivisionMesh \"catmull-clark\" [3] [0 1 2] [] [] [] [] \"P\" [0 0 5 1 0 5 0 1 5]\n"
        "Light \"PxrRectLight\" \"key\"\n"
        "WorldEnd\n", &handler));
    for (const char * request : { "Cone", "Cylinder", "Hyperboloid", "Paraboloid", "Disk", "Torus", "Points", "SubdivisionMesh", "Light" }) {
        BOOST_TEST(heard.said(std::string("do not draw ") + request), request);
    }
}

};  // namespace
};  // namespace v3d::moya::handling

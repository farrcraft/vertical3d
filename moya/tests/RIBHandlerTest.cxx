/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/trace/Scene.h>
#include <moya/libmoya/RIBHandler.h>
#include <moya/libmoya/Renderer.h>
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
 * Option "limits" is how a scene names the bucket and grid sizes. The standard's example file
 * opens with it.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_limits_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    BOOST_REQUIRE(read(
        "Option \"limits\" \"bucketsize\" [6 6]\n"
        "Option \"limits\" \"gridsize\" [18]\n", &handler));

    BOOST_CHECK_EQUAL(handler.context().bucketWidth(), 6U);
    BOOST_CHECK_EQUAL(handler.context().bucketHeight(), 6U);
    BOOST_CHECK_EQUAL(handler.context().gridSize(), 18U);
}

/**
 * A size pinned before the scene is read, as moya's --grid and --bucket pin theirs, survives a
 * scene that names its own with Option "limits". A size that is not pinned is replaced.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_pinned_limits_win_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    handler.context().pinGridSize(64);
    handler.context().pinBucketSize(8, 8);

    BOOST_REQUIRE(read(
        "Option \"limits\" \"bucketsize\" [6 6]\n"
        "Option \"limits\" \"gridsize\" [18]\n", &handler));
    BOOST_CHECK_EQUAL(handler.context().gridSize(), 64U);
    BOOST_CHECK_EQUAL(handler.context().bucketWidth(), 8U);

    // a pin of zero is refused, and pins nothing
    v3d::moya::Renderer other;
    v3d::moya::RIBHandler loose(&other);
    loose.context().pinGridSize(0);
    BOOST_REQUIRE(read("Option \"limits\" \"gridsize\" [18]\n", &loose));
    BOOST_CHECK_EQUAL(loose.context().gridSize(), 18U);
}

/**
 * Option "trace" "maxdepth" sets how deep trace() goes. A depth above sixteen is taken as
 * sixteen. A depth that is negative or not finite is not used, and the depth stays as it was.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_trace_depth_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

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
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    handler.output("data_out/override.png");

    BOOST_REQUIRE(read("Display \"scene-says.png\" \"file\" \"rgb\"\n", &handler));

    // nothing is written until WorldEnd, so the case reads the context and not the disk
    BOOST_CHECK_EQUAL(handler.context().displayName(), "data_out/override.png");
}

};  // namespace
};  // namespace v3d::moya::handling

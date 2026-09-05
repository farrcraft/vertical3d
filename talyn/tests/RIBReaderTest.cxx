/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../libtalyn/RIBReader.h"

#include <boost/make_shared.hpp>

BOOST_AUTO_TEST_CASE(ribreader_format_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBReader reader(rc);

    BOOST_REQUIRE_EQUAL(reader.read("data/format.rib"), true);

    // Format is the one request with a body, and it is what allocates the framebuffer
    auto framebuffer = rc->framebuffer();
    BOOST_REQUIRE(framebuffer);
    BOOST_CHECK_EQUAL(framebuffer->width(), 32u);
    BOOST_CHECK_EQUAL(framebuffer->height(), 16u);
}

BOOST_AUTO_TEST_CASE(ribreader_missing_file_test) {
    auto rc = boost::make_shared<v3d::talyn::RenderContext>();
    v3d::talyn::RIBReader reader(rc);

    BOOST_CHECK_EQUAL(reader.read("data/no-such-scene.rib"), false);
}

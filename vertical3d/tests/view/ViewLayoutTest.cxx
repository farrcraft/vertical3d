/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vertical3d/src/view/ViewLayout.h>

#include <string>

#include <boost/test/unit_test.hpp>

#include <boost/make_shared.hpp>

namespace {

/**
 * A layout config built straight from a json string, the way the asset loader would
 * hand one over.
 **/
boost::shared_ptr<v3d::asset::Json> config(const std::string& text) {
    boost::json::value parsed = boost::json::parse(text);
    return boost::make_shared<v3d::asset::Json>("layout", v3d::asset::Type::JsonDocument, parsed.as_object());
}

boost::shared_ptr<v3d::log::Logger> logger() {
    return boost::make_shared<v3d::log::Logger>();
}

/**
 * The quad split data/layout.json holds.
 **/
const char* const quad =
"{\"layout\": {\"name\": \"Quad Layout\", \"root\": {\"split\": \"vertical\", \"children\": ["
"{\"split\": \"horizontal\", \"children\": [{\"camera\": \"Front\"}, {\"camera\": \"Top\"}]},"
"{\"split\": \"horizontal\", \"children\": [{\"camera\": \"Left\"}, {\"camera\": \"Perspective\"}]}"
"]}}}";

};  // namespace

BOOST_AUTO_TEST_CASE(viewlayout_quad_test) {
    v3d::editor::ViewLayout layout(logger());
    BOOST_REQUIRE(layout.load(config(quad)));

    BOOST_CHECK_EQUAL(layout.name(), "Quad Layout");
    BOOST_REQUIRE_EQUAL(layout.views().size(), 4u);

    // the leaves come back in the order the document listed them, depth first
    BOOST_CHECK_EQUAL(layout.views()[0].camera, "Front");
    BOOST_CHECK_EQUAL(layout.views()[1].camera, "Top");
    BOOST_CHECK_EQUAL(layout.views()[2].camera, "Left");
    BOOST_CHECK_EQUAL(layout.views()[3].camera, "Perspective");

    layout.resize(1024, 768);

    // a vertical split stacks its two rows and each row puts its two views side by side
    BOOST_CHECK_EQUAL(layout.views()[0].region.x, 0.0f);
    BOOST_CHECK_EQUAL(layout.views()[0].region.y, 0.0f);
    BOOST_CHECK_EQUAL(layout.views()[0].region.z, 512.0f);
    BOOST_CHECK_EQUAL(layout.views()[0].region.w, 384.0f);

    BOOST_CHECK_EQUAL(layout.views()[1].region.x, 512.0f);
    BOOST_CHECK_EQUAL(layout.views()[1].region.y, 0.0f);

    BOOST_CHECK_EQUAL(layout.views()[2].region.x, 0.0f);
    BOOST_CHECK_EQUAL(layout.views()[2].region.y, 384.0f);

    BOOST_CHECK_EQUAL(layout.views()[3].region.x, 512.0f);
    BOOST_CHECK_EQUAL(layout.views()[3].region.y, 384.0f);
}

BOOST_AUTO_TEST_CASE(viewlayout_covers_the_window_test) {
    v3d::editor::ViewLayout layout(logger());
    BOOST_REQUIRE(layout.load(config(quad)));

    // an odd size does not divide evenly, and the last child of a split takes what integer
    // division left over - otherwise there is a seam of pixels no pass ever draws into
    layout.resize(1025, 769);

    BOOST_CHECK_EQUAL(layout.views()[0].region.z + layout.views()[1].region.z, 1025.0f);
    BOOST_CHECK_EQUAL(layout.views()[1].region.x, layout.views()[0].region.z);
    BOOST_CHECK_EQUAL(layout.views()[0].region.w + layout.views()[2].region.w, 769.0f);
    BOOST_CHECK_EQUAL(layout.views()[2].region.y, layout.views()[0].region.w);
}

BOOST_AUTO_TEST_CASE(viewlayout_hit_test) {
    v3d::editor::ViewLayout layout(logger());
    BOOST_REQUIRE(layout.load(config(quad)));
    layout.resize(1024, 768);

    BOOST_CHECK_EQUAL(layout.viewAt(10.0f, 10.0f), 0u);
    BOOST_CHECK_EQUAL(layout.viewAt(600.0f, 10.0f), 1u);
    BOOST_CHECK_EQUAL(layout.viewAt(10.0f, 600.0f), 2u);
    BOOST_CHECK_EQUAL(layout.viewAt(600.0f, 600.0f), 3u);

    // a border belongs to the view it starts, not the one it ends
    BOOST_CHECK_EQUAL(layout.viewAt(512.0f, 384.0f), 3u);
    BOOST_CHECK_EQUAL(layout.viewAt(511.0f, 383.0f), 0u);

    // and a point outside the window belongs to none
    BOOST_CHECK_EQUAL(layout.viewAt(2000.0f, 10.0f), layout.views().size());
}

BOOST_AUTO_TEST_CASE(viewlayout_single_view_test) {
    // a layout does not have to split at all - one viewport covering the window is a valid
    // one, and is what maximising a view will produce
    v3d::editor::ViewLayout layout(logger());
    BOOST_REQUIRE(layout.load(config("{\"layout\": {\"root\": {\"camera\": \"Perspective\"}}}")));

    BOOST_REQUIRE_EQUAL(layout.views().size(), 1u);
    layout.resize(800, 600);
    BOOST_CHECK_EQUAL(layout.views()[0].region.z, 800.0f);
    BOOST_CHECK_EQUAL(layout.views()[0].region.w, 600.0f);
}

BOOST_AUTO_TEST_CASE(viewlayout_rejects_test) {
    v3d::editor::ViewLayout layout(logger());

    // no layout at all
    BOOST_CHECK(!layout.load(config("{}")));
    // a layout with no root
    BOOST_CHECK(!layout.load(config("{\"layout\": {\"name\": \"empty\"}}")));
    // a split with nothing in it draws nothing, which is a config worth rejecting rather
    // than a window that comes up blank
    BOOST_CHECK(!layout.load(config("{\"layout\": {\"root\": {\"split\": \"vertical\", \"children\": []}}}")));
    // a node that is neither a camera nor a split
    BOOST_CHECK(!layout.load(config("{\"layout\": {\"root\": {\"name\": \"neither\"}}}")));
}

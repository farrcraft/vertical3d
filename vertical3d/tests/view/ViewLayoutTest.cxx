/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/kind/Json.h>
#include <api/log/Logger.h>
#include <vertical3d/src/view/ViewLayout.h>

#include <cmath>
#include <initializer_list>
#include <string>

#include <boost/test/unit_test.hpp>

#include <boost/make_shared.hpp>

namespace {

/**
 * A layout config built straight from a json string, the way the asset loader would
 * hand one over.
 **/
boost::shared_ptr<v3d::asset::kind::Json> config(const std::string& text) {
    boost::json::value parsed = boost::json::parse(text);
    return boost::make_shared<v3d::asset::kind::Json>("layout", parsed.as_object());
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

BOOST_AUTO_TEST_CASE(viewlayout_quad_test) {
    v3d::editor::ViewLayout layout(logger());
    BOOST_REQUIRE(layout.load(config(quad)));

    BOOST_CHECK_EQUAL(layout.name(), "Quad Layout");
    BOOST_REQUIRE_EQUAL(layout.views().size(), 4U);

    // the leaves come back in the order the document listed them, depth first
    BOOST_CHECK_EQUAL(layout.views()[0].camera, "Front");
    BOOST_CHECK_EQUAL(layout.views()[1].camera, "Top");
    BOOST_CHECK_EQUAL(layout.views()[2].camera, "Left");
    BOOST_CHECK_EQUAL(layout.views()[3].camera, "Perspective");

    layout.resize(1024, 768);

    // a vertical split stacks its two rows and each row puts its two views side by side
    BOOST_CHECK_EQUAL(layout.views()[0].region.x, 0.0F);
    BOOST_CHECK_EQUAL(layout.views()[0].region.y, 0.0F);
    BOOST_CHECK_EQUAL(layout.views()[0].region.z, 512.0F);
    BOOST_CHECK_EQUAL(layout.views()[0].region.w, 384.0F);

    BOOST_CHECK_EQUAL(layout.views()[1].region.x, 512.0F);
    BOOST_CHECK_EQUAL(layout.views()[1].region.y, 0.0F);

    BOOST_CHECK_EQUAL(layout.views()[2].region.x, 0.0F);
    BOOST_CHECK_EQUAL(layout.views()[2].region.y, 384.0F);

    BOOST_CHECK_EQUAL(layout.views()[3].region.x, 512.0F);
    BOOST_CHECK_EQUAL(layout.views()[3].region.y, 384.0F);
}

/**
 * A layout may show one camera profile in two views. Each view is named for its own pass, so
 * the second one is not drawn over the first.
 **/
BOOST_AUTO_TEST_CASE(viewlayout_names_each_view_of_one_profile_apart_test) {
    v3d::editor::ViewLayout layout(logger());
    BOOST_REQUIRE(layout.load(config(
        "{\"layout\": {\"root\": {\"split\": \"vertical\", \"children\": ["
        "{\"camera\": \"Perspective\"}, {\"camera\": \"Top\"}, {\"camera\": \"Perspective\"}"
        "]}}}")));

    BOOST_REQUIRE_EQUAL(layout.views().size(), 3U);
    BOOST_CHECK_EQUAL(layout.views()[0].name, "Perspective");
    BOOST_CHECK_EQUAL(layout.views()[1].name, "Top");
    BOOST_CHECK_EQUAL(layout.views()[2].name, "Perspective 2");
    BOOST_CHECK_EQUAL(layout.views()[2].camera, "Perspective");
}

BOOST_AUTO_TEST_CASE(viewlayout_covers_the_window_test) {
    v3d::editor::ViewLayout layout(logger());
    BOOST_REQUIRE(layout.load(config(quad)));

    // an odd size does not divide evenly, and the last child of a split takes what integer
    // division left over. Otherwise there would be a gap of pixels no pass draws into
    layout.resize(1025, 769);

    BOOST_CHECK_EQUAL(layout.views()[0].region.z + layout.views()[1].region.z, 1025.0F);
    BOOST_CHECK_EQUAL(layout.views()[1].region.x, layout.views()[0].region.z);
    BOOST_CHECK_EQUAL(layout.views()[0].region.w + layout.views()[2].region.w, 769.0F);
    BOOST_CHECK_EQUAL(layout.views()[2].region.y, layout.views()[0].region.w);
}

BOOST_AUTO_TEST_CASE(viewlayout_hit_test) {
    v3d::editor::ViewLayout layout(logger());
    BOOST_REQUIRE(layout.load(config(quad)));
    layout.resize(1024, 768);

    BOOST_CHECK_EQUAL(layout.viewAt(10.0F, 10.0F), 0U);
    BOOST_CHECK_EQUAL(layout.viewAt(600.0F, 10.0F), 1U);
    BOOST_CHECK_EQUAL(layout.viewAt(10.0F, 600.0F), 2U);
    BOOST_CHECK_EQUAL(layout.viewAt(600.0F, 600.0F), 3U);

    // a border belongs to the view it starts, not the one it ends
    BOOST_CHECK_EQUAL(layout.viewAt(512.0F, 384.0F), 3U);
    BOOST_CHECK_EQUAL(layout.viewAt(511.0F, 383.0F), 0U);

    // and a point outside the window belongs to none
    BOOST_CHECK_EQUAL(layout.viewAt(2000.0F, 10.0F), layout.views().size());
}

BOOST_AUTO_TEST_CASE(viewlayout_single_view_test) {
    // a layout does not have to split at all: one viewport covering the window is valid
    v3d::editor::ViewLayout layout(logger());
    BOOST_REQUIRE(layout.load(config("{\"layout\": {\"root\": {\"camera\": \"Perspective\"}}}")));

    BOOST_REQUIRE_EQUAL(layout.views().size(), 1U);
    layout.resize(800, 600);
    BOOST_CHECK_EQUAL(layout.views()[0].region.z, 800.0F);
    BOOST_CHECK_EQUAL(layout.views()[0].region.w, 600.0F);
}

BOOST_AUTO_TEST_CASE(viewlayout_rejects_test) {
    v3d::editor::ViewLayout layout(logger());

    // no layout at all
    BOOST_CHECK(!layout.load(config("{}")));
    // a layout with no root
    BOOST_CHECK(!layout.load(config("{\"layout\": {\"name\": \"empty\"}}")));
    // a split with nothing in it draws nothing, so the config is rejected rather than
    // bringing up a blank window
    BOOST_CHECK(!layout.load(config("{\"layout\": {\"root\": {\"split\": \"vertical\", \"children\": []}}}")));
    // a node that is neither a camera nor a split
    BOOST_CHECK(!layout.load(config("{\"layout\": {\"root\": {\"name\": \"neither\"}}}")));
    // a name, a camera or a split that is not a string is refused rather than thrown
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!layout.load(config("{\"layout\": {\"name\": 1, \"root\": {\"camera\": \"Top\"}}}"))));
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!layout.load(config("{\"layout\": {\"root\": {\"camera\": 3}}}"))));
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!layout.load(config(
        "{\"layout\": {\"root\": {\"split\": 2, \"children\": [{\"camera\": \"Top\"}]}}}"))));
}

BOOST_AUTO_TEST_CASE(viewlayout_rejects_a_nested_empty_split_test) {
    // an empty split beside a camera is refused, so it never takes the camera's region
    v3d::editor::ViewLayout layout(logger());
    BOOST_CHECK(!layout.load(config(
        "{\"layout\": {\"root\": {\"split\": \"horizontal\", \"children\": "
        "[{\"camera\": \"Top\"}, {\"split\": \"vertical\", \"children\": []}]}}}")));
}

BOOST_AUTO_TEST_CASE(viewlayout_shares_test) {
    // a split divides by its children's shares
    v3d::editor::ViewLayout layout(logger());
    BOOST_REQUIRE(layout.load(config(
        "{\"layout\": {\"root\": {\"split\": \"horizontal\", \"children\": ["
        "{\"camera\": \"Top\", \"share\": 1}, {\"camera\": \"Front\", \"share\": 3}]}}}")));
    layout.resize(800, 600);
    BOOST_CHECK_EQUAL(layout.views()[0].region.z, 200.0F);
    BOOST_CHECK_EQUAL(layout.views()[1].region.x, 200.0F);
    BOOST_CHECK_EQUAL(layout.views()[1].region.z, 600.0F);

    // a child with no share has a share of one
    BOOST_REQUIRE(layout.load(config(
        "{\"layout\": {\"root\": {\"split\": \"horizontal\", \"children\": ["
        "{\"camera\": \"Top\", \"share\": 2}, {\"camera\": \"Front\"}, {\"camera\": \"Left\"}]}}}")));
    layout.resize(800, 600);
    BOOST_CHECK_EQUAL(layout.views()[0].region.z, 400.0F);
    BOOST_CHECK_EQUAL(layout.views()[1].region.z, 200.0F);
    BOOST_CHECK_EQUAL(layout.views()[2].region.z, 200.0F);
}

BOOST_AUTO_TEST_CASE(viewlayout_rejects_a_bad_share_test) {
    v3d::editor::ViewLayout layout(logger());
    for (const char* share : {"0", "-1", "\"wide\""}) {
        BOOST_CHECK(!layout.load(config(std::string(
            "{\"layout\": {\"root\": {\"split\": \"horizontal\", \"children\": ["
            "{\"camera\": \"Top\", \"share\": ") + share + "}, {\"camera\": \"Front\"}]}}}")));
    }
}

BOOST_AUTO_TEST_CASE(viewlayout_divider_drag_test) {
    v3d::editor::ViewLayout layout(logger());
    BOOST_REQUIRE(layout.load(config(quad)));
    layout.resize(1024, 768);

    // a press away from every divider takes none, and one beside the top row's takes it
    BOOST_CHECK(!layout.grab(200.0F, 100.0F));
    BOOST_CHECK(!layout.grabbed());
    BOOST_CHECK(layout.grab(512.0F + v3d::editor::ViewLayout::dividerReach, 100.0F));

    // the divider moves the two views beside it, and the bottom row keeps its own
    layout.drag(612.0F, 300.0F);
    BOOST_CHECK_EQUAL(layout.views()[0].region.z, 612.0F);
    BOOST_CHECK_EQUAL(layout.views()[1].region.x, 612.0F);
    BOOST_CHECK_EQUAL(layout.views()[1].region.z, 412.0F);
    BOOST_CHECK_EQUAL(layout.views()[0].region.w, 384.0F);
    BOOST_CHECK_EQUAL(layout.views()[2].region.z, 512.0F);

    // dragged past the window's edge, each view keeps its floor
    layout.drag(-50.0F, 100.0F);
    BOOST_CHECK_EQUAL(layout.views()[0].region.z, std::floor(1024.0F * v3d::editor::ViewLayout::minimumShare));
    layout.drag(2000.0F, 100.0F);
    BOOST_CHECK_GE(layout.views()[1].region.z, std::floor(1024.0F * v3d::editor::ViewLayout::minimumShare));
    layout.drag(612.0F, 100.0F);
    layout.release();
    BOOST_CHECK(!layout.grabbed());

    // the shares outlast a new window size
    layout.resize(2048, 768);
    BOOST_CHECK_EQUAL(layout.views()[0].region.z, 1224.0F);
}

BOOST_AUTO_TEST_CASE(viewlayout_divider_crossing_test) {
    // where the rows' divider meets a row's, the outer split's divider is taken
    v3d::editor::ViewLayout layout(logger());
    BOOST_REQUIRE(layout.load(config(quad)));
    layout.resize(1024, 768);
    BOOST_REQUIRE(layout.grab(512.0F, 384.0F));
    layout.drag(512.0F, 484.0F);
    BOOST_CHECK_EQUAL(layout.views()[0].region.w, 484.0F);
    BOOST_CHECK_EQUAL(layout.views()[1].region.w, 484.0F);
    BOOST_CHECK_EQUAL(layout.views()[2].region.y, 484.0F);
    BOOST_CHECK_EQUAL(layout.views()[3].region.w, 284.0F);
    BOOST_CHECK_EQUAL(layout.views()[0].region.z, 512.0F);
}

};  // namespace

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <vertical3d/src/view/AreaLayout.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::AreaLayout;

boost::json::object parse(const std::string& text) {
    return boost::json::parse(text).as_object();
}

boost::shared_ptr<v3d::log::Logger> logger() {
    return boost::make_shared<v3d::log::Logger>();
}

/**
 * An area showing one viewport of a camera profile.
 **/
std::string view(const std::string& camera, const std::string& extra = "") {
    return R"({"editors": [{"kind": "viewport", "camera": ")" + camera + R"("}])" + extra + "}";
}

/**
 * The quad split data/layout.json holds: two rows of two.
 **/
std::string quad() {
    return R"({"name": "Modelling", "root": {"split": "vertical", "children": [)"
        R"({"split": "horizontal", "children": [)" + view("Front") + "," + view("Top") + "]},"
        R"({"split": "horizontal", "children": [)" + view("Left") + "," + view("Perspective") + "]}]}}";
}

/**
 * Two areas side by side.
 **/
std::string pair() {
    return R"({"root": {"split": "horizontal", "children": [)" + view("Front") + "," + view("Top") + "]}}";
}

/**
 * The quad over a 1024 by 768 window. With a 3 pixel gap the areas cover 3 to 1021 across and 3
 * to 765 down. The rows take 379 and 380 of the 759 left between them, and the columns 507 and
 * 508 of 1015.
 **/
struct Quad {
    Quad() : layout(logger()) {
        BOOST_REQUIRE(layout.load(parse(quad())));
        layout.resize(glm::vec4(0.0F, 0.0F, 1024.0F, 768.0F));
    }

    const glm::vec4& region(std::size_t area) const {
        return layout.areas()[area].region;
    }

    std::string camera(std::size_t area) const {
        return std::string(layout.areas()[area].editor().state.at("camera").as_string());
    }

    AreaLayout layout;
};

BOOST_AUTO_TEST_SUITE(area_layout_test)

BOOST_FIXTURE_TEST_CASE(the_quad_divides_the_window_with_gaps, Quad) {
    BOOST_CHECK_EQUAL(layout.name(), "Modelling");
    BOOST_REQUIRE_EQUAL(layout.areas().size(), 4U);

    // the areas come back in the order the document listed them, depth first
    BOOST_CHECK_EQUAL(camera(0), "Front");
    BOOST_CHECK_EQUAL(camera(1), "Top");
    BOOST_CHECK_EQUAL(camera(2), "Left");
    BOOST_CHECK_EQUAL(camera(3), "Perspective");
    BOOST_CHECK_EQUAL(layout.areas()[0].editor().kind, "viewport");

    BOOST_CHECK(region(0) == glm::vec4(3.0F, 3.0F, 507.0F, 379.0F));
    BOOST_CHECK(region(1) == glm::vec4(513.0F, 3.0F, 508.0F, 379.0F));
    BOOST_CHECK(region(2) == glm::vec4(3.0F, 385.0F, 507.0F, 380.0F));
    BOOST_CHECK(region(3) == glm::vec4(513.0F, 385.0F, 508.0F, 380.0F));
}

/**
 * An odd size does not divide evenly, and the last child of a split takes what rounding down
 * left over, so the areas and gaps still cover the window to its last pixel.
 **/
BOOST_FIXTURE_TEST_CASE(the_areas_cover_an_odd_window, Quad) {
    layout.resize(glm::vec4(0.0F, 0.0F, 1025.0F, 769.0F));
    BOOST_CHECK_EQUAL(region(1).x + region(1).z + 3.0F, 1025.0F);
    BOOST_CHECK_EQUAL(region(1).x, region(0).x + region(0).z + 3.0F);
    BOOST_CHECK_EQUAL(region(2).y + region(2).w + 3.0F, 769.0F);
    BOOST_CHECK_EQUAL(region(2).y, region(0).y + region(0).w + 3.0F);
}

BOOST_FIXTURE_TEST_CASE(a_point_falls_in_the_area_under_it_and_a_gap_in_none, Quad) {
    BOOST_CHECK_EQUAL(layout.areaAt(10.0F, 10.0F), 0U);
    BOOST_CHECK_EQUAL(layout.areaAt(600.0F, 10.0F), 1U);
    BOOST_CHECK_EQUAL(layout.areaAt(10.0F, 600.0F), 2U);
    BOOST_CHECK_EQUAL(layout.areaAt(600.0F, 600.0F), 3U);

    BOOST_CHECK_EQUAL(layout.areaAt(511.0F, 10.0F), 4U);
    BOOST_CHECK_EQUAL(layout.areaAt(1.0F, 10.0F), 4U);
    BOOST_CHECK_EQUAL(layout.areaAt(2000.0F, 10.0F), 4U);
}

BOOST_AUTO_TEST_CASE(one_area_needs_no_split) {
    AreaLayout layout(logger());
    BOOST_REQUIRE(layout.load(parse(R"({"root": )" + view("Perspective") + "}")));
    layout.resize(glm::vec4(0.0F, 0.0F, 800.0F, 600.0F));
    BOOST_REQUIRE_EQUAL(layout.areas().size(), 1U);
    BOOST_CHECK(layout.areas()[0].region == glm::vec4(3.0F, 3.0F, 794.0F, 594.0F));
}

BOOST_AUTO_TEST_CASE(a_layout_that_cannot_be_read_is_refused) {
    AreaLayout layout(logger());
    const std::initializer_list<std::string> refused = {
        // no root
        R"({"name": "empty"})",
        // a split with nothing in it
        R"({"root": {"split": "vertical", "children": []}})",
        // an empty split beside an area
        R"({"root": {"split": "horizontal", "children": [)" + view("Top") + R"(, {"split": "vertical", "children": []}]}})",
        // a node that is neither an area nor a split
        R"({"root": {"name": "neither"}})",
        // a name or a split that is not a string
        R"({"name": 1, "root": )" + view("Top") + "}",
        R"({"root": {"split": 2, "children": [)" + view("Top") + "]}}",
        // an area with no editors, or an editor that names no kind
        R"({"root": {"editors": []}})",
        R"({"root": {"editors": [{"camera": "Top"}]}})",
        R"({"root": {"editors": [{"kind": ""}]}})",
        R"({"root": {"editors": ["viewport"]}})",
        // a chosen editor the area does not hold
        R"({"root": )" + view("Top", R"(, "chosen": 1)") + "}",
        R"({"root": )" + view("Top", R"(, "chosen": -1)") + "}",
    };
    for (const std::string& text : refused) {
        BOOST_CHECK_NO_THROW(BOOST_CHECK_MESSAGE(!layout.load(parse(text)), text));
    }
    for (const char* share : { "0", "-1", "\"wide\"" }) {
        BOOST_CHECK(!layout.load(parse(R"({"root": {"split": "horizontal", "children": [)" +
            view("Top", std::string(R"(, "share": )") + share) + "," + view("Front") + "]}}")));
    }
}

/**
 * A layout that cannot be read leaves the one held as it was.
 **/
BOOST_FIXTURE_TEST_CASE(a_refused_layout_keeps_the_last, Quad) {
    BOOST_CHECK(!layout.load(parse(R"({"root": {"editors": []}})")));
    BOOST_CHECK_EQUAL(layout.areas().size(), 4U);
    BOOST_CHECK_EQUAL(layout.name(), "Modelling");
    BOOST_CHECK(region(3) == glm::vec4(513.0F, 385.0F, 508.0F, 380.0F));
}

/**
 * A split divides by its children's shares, and a child with none has a share of one.
 **/
BOOST_AUTO_TEST_CASE(a_split_divides_by_shares) {
    AreaLayout layout(logger());
    layout.metrics({ .gap = 0.0F });
    BOOST_REQUIRE(layout.load(parse(R"({"root": {"split": "horizontal", "children": [)" +
        view("Top", R"(, "share": 1)") + "," + view("Front", R"(, "share": 3)") + "]}}")));
    layout.resize(glm::vec4(0.0F, 0.0F, 800.0F, 600.0F));
    BOOST_CHECK_EQUAL(layout.areas()[0].region.z, 200.0F);
    BOOST_CHECK_EQUAL(layout.areas()[1].region.x, 200.0F);
    BOOST_CHECK_EQUAL(layout.areas()[1].region.z, 600.0F);

    BOOST_REQUIRE(layout.load(parse(R"({"root": {"split": "horizontal", "children": [)" +
        view("Top", R"(, "share": 2)") + "," + view("Front") + "," + view("Left") + "]}}")));
    BOOST_CHECK_EQUAL(layout.areas()[0].region.z, 400.0F);
    BOOST_CHECK_EQUAL(layout.areas()[1].region.z, 200.0F);
    BOOST_CHECK_EQUAL(layout.areas()[2].region.z, 200.0F);
}

/**
 * What save() writes, load() reads back to the same areas, editors and shares.
 **/
BOOST_FIXTURE_TEST_CASE(a_saved_layout_reads_back_the_same, Quad) {
    layout.grab(511.0F, 100.0F);
    layout.drag(700.0F, 100.0F);
    layout.release();
    BOOST_REQUIRE(layout.add(3, parse(R"({"kind": "outliner", "filter": "mesh"})")));

    AreaLayout reread(logger());
    BOOST_REQUIRE(reread.load(layout.save()));
    reread.resize(glm::vec4(0.0F, 0.0F, 1024.0F, 768.0F));
    BOOST_CHECK_EQUAL(reread.name(), "Modelling");
    BOOST_REQUIRE_EQUAL(reread.areas().size(), 4U);
    for (std::size_t area = 0; area < 4; area++) {
        BOOST_CHECK(reread.areas()[area].region == region(area));
        BOOST_CHECK(reread.areas()[area].editors.size() == layout.areas()[area].editors.size());
        BOOST_CHECK(reread.areas()[area].editor().state == layout.areas()[area].editor().state);
    }
    BOOST_CHECK_EQUAL(reread.areas()[3].chosen, 1U);
    BOOST_CHECK_EQUAL(reread.areas()[3].editor().kind, "outliner");
}

BOOST_FIXTURE_TEST_CASE(a_divider_is_taken_within_reach_of_its_gap, Quad) {
    // the top row's gap runs from 510 to 513, and is taken 4 pixels either side of it
    const std::optional<AreaLayout::Divider> found = layout.divider(506.0F, 100.0F);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found.value_or(AreaLayout::Divider()).region == glm::vec4(510.0F, 3.0F, 3.0F, 379.0F));
    BOOST_CHECK(!found.value_or(AreaLayout::Divider{ .across = true }).across);
    BOOST_CHECK(layout.divider(517.0F, 100.0F));
    BOOST_CHECK(!layout.divider(505.0F, 100.0F));
    BOOST_CHECK(!layout.divider(518.0F, 100.0F));
    BOOST_CHECK(!layout.divider(200.0F, 100.0F));

    BOOST_CHECK(!layout.grab(200.0F, 100.0F));
    BOOST_CHECK(!layout.grabbed());
    BOOST_CHECK(layout.grab(511.0F, 100.0F));
    BOOST_REQUIRE(layout.held().has_value());
    BOOST_CHECK(layout.held().value_or(AreaLayout::Divider()).region == found.value_or(AreaLayout::Divider()).region);
}

/**
 * Where the rows' divider meets a row's, the outer split's is taken, and it runs across.
 **/
BOOST_FIXTURE_TEST_CASE(the_outer_divider_wins_where_two_meet, Quad) {
    BOOST_REQUIRE(layout.grab(511.0F, 383.0F));
    BOOST_CHECK(layout.held().value_or(AreaLayout::Divider()).across);
    layout.drag(511.0F, 484.0F);
    BOOST_CHECK_EQUAL(region(0).w, region(1).w);
    BOOST_CHECK_EQUAL(region(0).w, 479.0F);
    BOOST_CHECK_EQUAL(region(2).y, 485.0F);
    BOOST_CHECK_EQUAL(region(0).z, 507.0F);
}

/**
 * Dragging moves the two areas beside the divider, and the other row keeps its own. Released,
 * the shares outlast a new window size.
 **/
BOOST_FIXTURE_TEST_CASE(a_dragged_divider_trades_room_between_two_areas, Quad) {
    BOOST_REQUIRE(layout.grab(511.0F, 100.0F));
    layout.drag(612.0F, 300.0F);
    BOOST_CHECK_EQUAL(region(0).z, 607.0F);
    BOOST_CHECK_EQUAL(region(1).x, 613.0F);
    BOOST_CHECK_EQUAL(region(1).x + region(1).z, 1021.0F);
    BOOST_CHECK_EQUAL(region(0).w, 379.0F);
    BOOST_CHECK_EQUAL(region(2).z, 507.0F);
    layout.release();
    BOOST_CHECK(!layout.grabbed());

    layout.resize(glm::vec4(0.0F, 0.0F, 2048.0F, 768.0F));
    BOOST_CHECK_CLOSE(region(0).z / (region(0).z + region(1).z), 607.0F / 1015.0F, 0.2F);
}

/**
 * Neither area is dragged smaller than its least size, past either edge of the window.
 **/
BOOST_FIXTURE_TEST_CASE(a_dragged_area_keeps_its_least_size, Quad) {
    BOOST_REQUIRE(layout.grab(511.0F, 100.0F));
    layout.drag(-50.0F, 100.0F);
    BOOST_CHECK_GE(region(0).z, 67.0F);
    BOOST_CHECK_LE(region(0).z, 68.0F);
    layout.drag(2000.0F, 100.0F);
    BOOST_CHECK_GE(region(1).z, 68.0F);
    BOOST_CHECK_LE(region(1).z, 69.0F);
}

/**
 * A split beside the divider needs room for each of its own areas when it runs the same way,
 * and for its largest when it runs across.
 **/
BOOST_AUTO_TEST_CASE(a_split_keeps_the_least_size_of_what_it_holds) {
    AreaLayout layout(logger());
    BOOST_REQUIRE(layout.load(parse(R"({"root": {"split": "horizontal", "children": [)" + view("Front") +
        R"(, {"split": "horizontal", "children": [)" + view("Top") + "," + view("Left") + "]}]}}")));
    layout.resize(glm::vec4(0.0F, 0.0F, 1024.0F, 768.0F));
    BOOST_REQUIRE(layout.grab(layout.areas()[1].region.x - 1.0F, 100.0F));
    layout.drag(2000.0F, 100.0F);
    // the inner split needs 68 + 3 + 68, from 1021 back
    BOOST_CHECK_GE(layout.areas()[1].region.x, 1021.0F - 139.0F - 1.0F);
    BOOST_CHECK_GE(layout.areas()[1].region.z, 67.0F);
    BOOST_CHECK_GE(layout.areas()[2].region.z, 67.0F);
}

/**
 * With snapping, the divider lands on the nearest tenth of its split.
 **/
BOOST_FIXTURE_TEST_CASE(a_snapped_divider_lands_on_a_tenth, Quad) {
    BOOST_REQUIRE(layout.grab(511.0F, 100.0F));
    layout.drag(400.0F, 100.0F, true);
    // the split runs 1018 pixels from 3, so its fourth tenth is at 410.2
    const float centre = region(0).x + region(0).z + 1.5F;
    BOOST_CHECK_LE(std::abs(centre - 410.2F), 1.0F);
}

/**
 * Cancelling puts the divider back where it was taken.
 **/
BOOST_FIXTURE_TEST_CASE(a_cancelled_drag_puts_the_divider_back, Quad) {
    BOOST_REQUIRE(layout.grab(511.0F, 100.0F));
    layout.drag(800.0F, 100.0F);
    BOOST_REQUIRE(region(0).z > 507.0F);
    layout.cancel();
    BOOST_CHECK(!layout.grabbed());
    BOOST_CHECK(region(0) == glm::vec4(3.0F, 3.0F, 507.0F, 379.0F));
    BOOST_CHECK(region(1) == glm::vec4(513.0F, 3.0F, 508.0F, 379.0F));
}

/**
 * Evening a divider gives the two areas beside it the same room.
 **/
BOOST_FIXTURE_TEST_CASE(an_evened_divider_divides_its_two_alike, Quad) {
    BOOST_REQUIRE(layout.grab(511.0F, 100.0F));
    layout.drag(800.0F, 100.0F);
    layout.release();
    BOOST_CHECK(!layout.even(200.0F, 100.0F));
    BOOST_REQUIRE(layout.even(region(1).x - 1.0F, 100.0F));
    BOOST_CHECK(region(0) == glm::vec4(3.0F, 3.0F, 507.0F, 379.0F));
}

/**
 * A maximised area takes the whole region, the others have none, and no divider can be taken.
 * Maximising it again gives every area its place back.
 **/
BOOST_FIXTURE_TEST_CASE(a_maximised_area_takes_the_window, Quad) {
    layout.maximise(1);
    BOOST_REQUIRE(layout.maximised());
    BOOST_CHECK_EQUAL(layout.maximised().value_or(4U), 1U);
    BOOST_CHECK(region(1) == glm::vec4(3.0F, 3.0F, 1018.0F, 762.0F));
    BOOST_CHECK(region(0) == glm::vec4(0.0F));
    BOOST_CHECK_EQUAL(layout.areaAt(10.0F, 10.0F), 1U);
    BOOST_CHECK(!layout.divider(511.0F, 100.0F));
    BOOST_CHECK(!layout.grab(511.0F, 100.0F));

    layout.maximise(1);
    BOOST_CHECK(!layout.maximised());
    BOOST_CHECK(region(1) == glm::vec4(513.0F, 3.0F, 508.0F, 379.0F));
}

/**
 * A split puts a copy of the editor shown on the edge asked for. A split along a row that runs
 * the same way joins that row rather than nesting.
 **/
BOOST_FIXTURE_TEST_CASE(a_split_divides_an_area_in_two, Quad) {
    const std::size_t id = layout.areas()[0].id;
    const std::size_t made = layout.split(0, AreaLayout::Edge::Right);
    BOOST_REQUIRE_EQUAL(layout.areas().size(), 5U);
    BOOST_CHECK_EQUAL(made, 1U);
    BOOST_CHECK_EQUAL(layout.find(id), 0U);
    BOOST_CHECK_EQUAL(camera(1), "Front");
    BOOST_CHECK_NE(layout.areas()[1].editor().id, layout.areas()[0].editor().id);
    // the two halve the old area's share, and the row's third keeps the other half
    BOOST_CHECK_EQUAL(region(0).x, 3.0F);
    BOOST_CHECK_EQUAL(region(0).z, 253.0F);
    BOOST_CHECK_EQUAL(region(1).z, 253.0F);
    BOOST_CHECK_EQUAL(region(1).x + region(1).z + 3.0F, region(2).x);
    BOOST_CHECK_EQUAL(region(2).z, 506.0F);

    const boost::json::object saved = layout.save();
    BOOST_CHECK_EQUAL(saved.at("root").at("children").at(0).at("children").as_array().size(), 3U);

    // a split to the top puts the new area first, and renumbers the old one
    const std::size_t top = layout.split(4, AreaLayout::Edge::Top);
    BOOST_CHECK_EQUAL(top, 4U);
    BOOST_CHECK_EQUAL(camera(5), "Perspective");
    BOOST_CHECK_LT(region(4).y, region(5).y);
}

BOOST_AUTO_TEST_CASE(an_area_too_small_for_two_is_not_split) {
    AreaLayout layout(logger());
    BOOST_REQUIRE(layout.load(parse(R"({"root": )" + view("Top") + "}")));
    layout.resize(glm::vec4(0.0F, 0.0F, 140.0F, 600.0F));
    BOOST_CHECK_EQUAL(layout.split(0, AreaLayout::Edge::Left), 1U);
    BOOST_CHECK_EQUAL(layout.areas().size(), 1U);
    BOOST_CHECK_EQUAL(layout.split(0, AreaLayout::Edge::Bottom), 1U);
    BOOST_CHECK_EQUAL(layout.areas().size(), 2U);
}

/**
 * Joining gives one area's room to the area beside it in the same split, and a split left with
 * one child is that child.
 **/
BOOST_FIXTURE_TEST_CASE(a_join_gives_an_area_its_neighbours_room, Quad) {
    BOOST_CHECK(!layout.join(0, 3));
    BOOST_CHECK(!layout.join(0, 2));
    BOOST_CHECK(!layout.join(0, 0));
    BOOST_REQUIRE(layout.join(0, 1));
    BOOST_REQUIRE_EQUAL(layout.areas().size(), 3U);
    BOOST_CHECK_EQUAL(camera(0), "Front");
    BOOST_CHECK(region(0) == glm::vec4(3.0F, 3.0F, 1018.0F, 379.0F));
    BOOST_CHECK(region(1) == glm::vec4(3.0F, 385.0F, 507.0F, 380.0F));

    BOOST_REQUIRE(layout.join(2, 1));
    BOOST_REQUIRE(layout.join(0, 1));
    BOOST_REQUIRE_EQUAL(layout.areas().size(), 1U);
    BOOST_CHECK(region(0) == glm::vec4(3.0F, 3.0F, 1018.0F, 762.0F));
    BOOST_CHECK(layout.save().at("root").as_object().contains("editors"));
}

/**
 * Swapping exchanges what two areas hold, and each area keeps its id and its place.
 **/
BOOST_FIXTURE_TEST_CASE(a_swap_exchanges_what_two_areas_hold, Quad) {
    const std::size_t first = layout.areas()[0].id;
    BOOST_REQUIRE(layout.swap(0, 3));
    BOOST_CHECK_EQUAL(camera(0), "Perspective");
    BOOST_CHECK_EQUAL(camera(3), "Front");
    BOOST_CHECK_EQUAL(layout.areas()[0].id, first);
    BOOST_CHECK(!layout.swap(0, 4));
}

/**
 * An area's editors are its tabs: one added is shown, an only editor is never closed, and
 * closing the one shown shows the one before it.
 **/
BOOST_FIXTURE_TEST_CASE(an_area_holds_editors_as_tabs, Quad) {
    BOOST_CHECK(!layout.close(0, 0));
    BOOST_CHECK(!layout.add(0, parse(R"({"filter": "mesh"})")));
    BOOST_CHECK(!layout.add(4, parse(R"({"kind": "outliner"})")));

    const std::optional<std::size_t> outliner = layout.add(0, parse(R"({"kind": "outliner"})"));
    const std::optional<std::size_t> console = layout.add(0, parse(R"({"kind": "console"})"));
    BOOST_REQUIRE(outliner && console);
    BOOST_CHECK_NE(*outliner, *console);
    BOOST_CHECK_EQUAL(layout.areas()[0].chosen, 2U);
    BOOST_CHECK_EQUAL(layout.areas()[0].editor().kind, "console");

    BOOST_REQUIRE(layout.choose(0, 1));
    BOOST_CHECK_EQUAL(layout.areas()[0].editor().id, *outliner);
    BOOST_CHECK(!layout.choose(0, 3));

    // closing one before the one shown keeps showing the same editor
    BOOST_REQUIRE(layout.close(0, 0));
    BOOST_CHECK_EQUAL(layout.areas()[0].editor().id, *outliner);
    // closing the one shown shows the one before it, or the first
    BOOST_REQUIRE(layout.close(0, 0));
    BOOST_CHECK_EQUAL(layout.areas()[0].editor().id, *console);
    BOOST_CHECK(!layout.close(0, 0));
}

/**
 * An entry of an editor's state can be set by the editor's id, and is saved. Its kind cannot.
 **/
BOOST_FIXTURE_TEST_CASE(an_editors_state_can_be_set, Quad) {
    const std::size_t id = layout.areas()[1].editor().id;
    BOOST_REQUIRE(layout.set(id, "camera", "Bottom"));
    BOOST_CHECK_EQUAL(camera(1), "Bottom");
    BOOST_CHECK(!layout.set(id, "kind", "outliner"));
    BOOST_CHECK_EQUAL(layout.areas()[1].editor().kind, "viewport");
    BOOST_CHECK(!layout.set(9999, "camera", "Top"));

    AreaLayout again(logger());
    BOOST_REQUIRE(again.load(layout.save()));
    BOOST_CHECK_EQUAL(std::string(again.areas()[1].editor().state.at("camera").as_string()), "Bottom");
}

/**
 * A moved editor is shown in its new area, and an area it leaves empty goes.
 **/
BOOST_FIXTURE_TEST_CASE(a_moved_editor_takes_its_area_with_it_when_alone, Quad) {
    const std::size_t top = layout.areas()[1].editor().id;
    BOOST_REQUIRE(layout.move(1, 0, 0));
    BOOST_REQUIRE_EQUAL(layout.areas().size(), 3U);
    BOOST_CHECK_EQUAL(layout.areas()[0].editors.size(), 2U);
    BOOST_CHECK_EQUAL(layout.areas()[0].editor().id, top);
    BOOST_CHECK(region(0) == glm::vec4(3.0F, 3.0F, 1018.0F, 379.0F));
    BOOST_CHECK(!layout.move(0, 0, 0));
}

/**
 * A tab dropped on an area's edge makes a new area there. The area it left keeps its place
 * while it holds another editor, and goes when it holds none.
 **/
BOOST_FIXTURE_TEST_CASE(a_dropped_tab_splits_the_area_it_lands_on, Quad) {
    BOOST_REQUIRE(layout.add(0, parse(R"({"kind": "outliner"})")));
    BOOST_REQUIRE(layout.drop(0, 1, 1, AreaLayout::Edge::Bottom));
    BOOST_REQUIRE_EQUAL(layout.areas().size(), 5U);
    BOOST_CHECK_EQUAL(layout.areas()[0].editors.size(), 1U);
    BOOST_CHECK_EQUAL(camera(1), "Top");
    BOOST_CHECK_EQUAL(layout.areas()[2].editor().kind, "outliner");
    BOOST_CHECK_GT(region(2).y, region(1).y);
    BOOST_CHECK_EQUAL(region(2).x, region(1).x);

    // an only editor dropped on its own area stays
    BOOST_CHECK(!layout.drop(0, 0, 0, AreaLayout::Edge::Left));

    // and dropped on another area, it takes its own area with it
    BOOST_REQUIRE(layout.drop(0, 0, 1, AreaLayout::Edge::Left));
    BOOST_REQUIRE_EQUAL(layout.areas().size(), 5U);
    BOOST_CHECK_EQUAL(camera(0), "Front");
    BOOST_CHECK_EQUAL(camera(1), "Top");
    BOOST_CHECK_EQUAL(region(0).y, region(1).y);
    BOOST_CHECK_LT(region(0).x, region(1).x);
}

/**
 * A change to the tree lets go of a held divider and gives a maximised area's room back.
 **/
BOOST_FIXTURE_TEST_CASE(a_change_to_the_tree_restores_the_layout, Quad) {
    BOOST_REQUIRE(layout.grab(511.0F, 100.0F));
    layout.split(3, AreaLayout::Edge::Right);
    BOOST_CHECK(!layout.grabbed());

    layout.maximise(0);
    layout.split(0, AreaLayout::Edge::Bottom);
    BOOST_CHECK(!layout.maximised());
    BOOST_CHECK_EQUAL(region(0).x, 3.0F);
    BOOST_CHECK_GT(region(1).z, 0.0F);
}

/**
 * The gaps are the margin's four sides and each divider, and the margin alone while an area is
 * maximised.
 **/
BOOST_FIXTURE_TEST_CASE(the_gaps_are_the_margin_and_the_dividers, Quad) {
    const std::vector<glm::vec4> gaps = layout.gaps();
    BOOST_REQUIRE_EQUAL(gaps.size(), 7U);
    BOOST_CHECK(gaps[0] == glm::vec4(0.0F, 0.0F, 1024.0F, 3.0F));
    BOOST_CHECK(gaps[1] == glm::vec4(0.0F, 765.0F, 1024.0F, 3.0F));
    BOOST_CHECK(gaps[2] == glm::vec4(0.0F, 3.0F, 3.0F, 762.0F));
    BOOST_CHECK(gaps[3] == glm::vec4(1021.0F, 3.0F, 3.0F, 762.0F));
    BOOST_CHECK(std::ranges::find(gaps, glm::vec4(3.0F, 382.0F, 1018.0F, 3.0F)) != gaps.end());
    BOOST_CHECK(std::ranges::find(gaps, glm::vec4(510.0F, 3.0F, 3.0F, 379.0F)) != gaps.end());
    BOOST_CHECK(std::ranges::find(gaps, glm::vec4(510.0F, 385.0F, 3.0F, 380.0F)) != gaps.end());

    layout.maximise(2);
    BOOST_CHECK_EQUAL(layout.gaps().size(), 4U);
}

/**
 * The neighbour on an edge is the area across the gap from that edge's middle, and there is
 * none on the window's edge.
 **/
BOOST_FIXTURE_TEST_CASE(a_neighbour_is_across_the_gap, Quad) {
    BOOST_CHECK_EQUAL(layout.neighbour(0, AreaLayout::Edge::Right), 1U);
    BOOST_CHECK_EQUAL(layout.neighbour(0, AreaLayout::Edge::Bottom), 2U);
    BOOST_CHECK_EQUAL(layout.neighbour(3, AreaLayout::Edge::Left), 2U);
    BOOST_CHECK_EQUAL(layout.neighbour(3, AreaLayout::Edge::Top), 1U);
    BOOST_CHECK_EQUAL(layout.neighbour(0, AreaLayout::Edge::Left), 4U);
    BOOST_CHECK_EQUAL(layout.neighbour(1, AreaLayout::Edge::Top), 4U);
    BOOST_CHECK_EQUAL(layout.neighbour(9, AreaLayout::Edge::Top), 4U);
}

/**
 * An area's editor draws under a strip of tabs while the area holds two or more editors, and
 * in the whole area otherwise.
 **/
BOOST_FIXTURE_TEST_CASE(tabs_take_a_strip_off_the_top, Quad) {
    layout.metrics({ .strip = 28.0F });
    BOOST_CHECK(layout.areas()[0].content == region(0));
    BOOST_REQUIRE(layout.add(0, parse(R"({"kind": "outliner"})")));
    BOOST_CHECK(layout.areas()[0].content == glm::vec4(3.0F, 31.0F, 507.0F, 351.0F));
    BOOST_REQUIRE(layout.close(0, 1));
    BOOST_CHECK(layout.areas()[0].content == region(0));

    // a swap carries the strip with the editors
    BOOST_REQUIRE(layout.add(0, parse(R"({"kind": "outliner"})")));
    BOOST_REQUIRE(layout.swap(0, 3));
    BOOST_CHECK(layout.areas()[0].content == region(0));
    BOOST_CHECK_EQUAL(layout.areas()[3].content.y, region(3).y + 28.0F);
}

/**
 * The metrics set the gap, the reach and the least size.
 **/
BOOST_AUTO_TEST_CASE(the_metrics_set_the_gap) {
    AreaLayout layout(logger());
    BOOST_REQUIRE(layout.load(parse(pair())));
    layout.resize(glm::vec4(0.0F, 0.0F, 200.0F, 100.0F));
    layout.metrics({ .gap = 6.0F, .reach = 2.0F, .least = 20.0F });
    BOOST_CHECK(layout.areas()[0].region == glm::vec4(6.0F, 6.0F, 91.0F, 88.0F));
    BOOST_CHECK_EQUAL(layout.areas()[1].region.x, 103.0F);
    BOOST_CHECK(!layout.divider(94.0F, 50.0F));
    BOOST_CHECK(layout.divider(95.0F, 50.0F));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

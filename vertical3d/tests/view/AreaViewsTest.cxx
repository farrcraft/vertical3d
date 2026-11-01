/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <string>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::view::AreaLayout;
using v3d::editor::view::AreaViews;

/**
 * Three areas in a row: a viewport, an outliner, and a viewport of a camera with no profile.
 * The make refuses that camera, as the editor refuses one its profiles do not hold.
 **/
struct Row {
    explicit Row(const char* third = "Top") :
        layout(boost::make_shared<AreaLayout>(boost::make_shared<v3d::log::Logger>())) {
        const std::string text = std::string(R"({"root": {"split": "horizontal", "children": [)")
            + R"({"editors": [{"kind": "viewport", "camera": "Front"}]},)"
            + R"({"editors": [{"kind": "outliner"}]},)"
            + R"({"editors": [{"kind": "viewport", "camera": ")" + third + R"("}]}]}})";
        BOOST_REQUIRE(layout->load(boost::json::parse(text).as_object()));
        layout->resize(glm::vec4(0.0F, 0.0F, 600.0F, 400.0F));
        views = boost::make_shared<AreaViews>(layout, [this](const AreaLayout::Editor& editor) -> boost::shared_ptr<v3d::editor::view::ViewPort> {
            made++;
            const std::string camera(editor.state.at("camera").as_string());
            if (camera == "Missing") {
                return nullptr;
            }
            return boost::make_shared<v3d::editor::view::ViewPort>(camera, v3d::type::camera::Profile(camera));
        });
    }

    boost::shared_ptr<AreaLayout> layout;
    boost::shared_ptr<AreaViews> views;
    int made = 0;
};

BOOST_AUTO_TEST_SUITE(area_views_test)

/**
 * Each viewport editor has a view, given its area's region. An editor of another kind has none.
 **/
BOOST_FIXTURE_TEST_CASE(each_viewport_editor_has_a_view, Row) {
    BOOST_REQUIRE(views->sync());
    BOOST_REQUIRE_EQUAL(views->shown().size(), 2U);
    BOOST_CHECK_EQUAL(views->shown()[0]->name(), "Front");
    BOOST_CHECK_EQUAL(views->shown()[1]->name(), "Top");
    BOOST_CHECK(views->shown()[0]->region() == layout->areas()[0].region);
    BOOST_CHECK(views->shown()[1]->region() == layout->areas()[2].region);
    BOOST_CHECK(views->of(0) == views->shown()[0]);
    BOOST_CHECK(!views->of(1));
    BOOST_CHECK(!views->of(3));
}

/**
 * A view lasts as long as its editor, so syncing again makes nothing new, and a view keeps its
 * state while another tab is shown.
 **/
BOOST_FIXTURE_TEST_CASE(a_view_lasts_as_long_as_its_editor, Row) {
    BOOST_REQUIRE(views->sync());
    const boost::shared_ptr<v3d::editor::view::ViewPort> front = views->of(0);
    BOOST_REQUIRE(views->sync());
    BOOST_CHECK_EQUAL(made, 2);

    BOOST_REQUIRE(layout->add(0, boost::json::parse(R"({"kind": "console"})").as_object()));
    BOOST_REQUIRE(views->sync());
    BOOST_CHECK_EQUAL(views->shown().size(), 1U);
    BOOST_CHECK(views->editor(layout->areas()[0].editors[0].id) == front);

    BOOST_REQUIRE(layout->choose(0, 0));
    BOOST_REQUIRE(views->sync());
    BOOST_CHECK(views->of(0) == front);
    BOOST_CHECK_EQUAL(made, 2);
}

/**
 * A view whose editor is closed goes, and a copy made by a split gets a view of its own.
 **/
BOOST_FIXTURE_TEST_CASE(views_follow_the_editors, Row) {
    BOOST_REQUIRE(views->sync());
    const std::size_t front = layout->areas()[0].editor().id;
    BOOST_REQUIRE(layout->join(1, 0));
    BOOST_REQUIRE(views->sync());
    BOOST_CHECK(!views->editor(front));
    BOOST_CHECK_EQUAL(views->shown().size(), 1U);

    const std::size_t below = layout->split(1, AreaLayout::Edge::Bottom);
    BOOST_REQUIRE(below < layout->areas().size());
    BOOST_REQUIRE(views->sync());
    BOOST_CHECK_EQUAL(views->shown().size(), 2U);
    BOOST_CHECK(views->of(below) != views->of(1));
}

/**
 * The areas behind a maximised one show nothing.
 **/
BOOST_FIXTURE_TEST_CASE(a_maximised_area_shows_its_view_alone, Row) {
    layout->maximise(2);
    BOOST_REQUIRE(views->sync());
    BOOST_REQUIRE_EQUAL(views->shown().size(), 1U);
    BOOST_CHECK_EQUAL(views->shown()[0]->name(), "Top");
    BOOST_CHECK(views->shown()[0]->region() == layout->areas()[2].region);
}

/**
 * A viewport the make refuses has no view, and the sync says so.
 **/
BOOST_AUTO_TEST_CASE(a_refused_viewport_fails_the_sync) {
    const Row row("Missing");
    BOOST_CHECK(!row.views->sync());
    BOOST_CHECK_EQUAL(row.views->shown().size(), 1U);
    BOOST_CHECK(!row.views->of(2));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

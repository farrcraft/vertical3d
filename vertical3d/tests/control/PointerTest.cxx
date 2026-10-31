/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Router.h>
#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <api/ui/Component.h>
#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/input/Cursor.h>
#include <vertical3d/src/control/Pointer.h>
#include <vertical3d/src/tool/Router.h>
#include <vertical3d/src/tool/Tool.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

/**
 * A tool that takes every press, and remembers the view it was given and whether it is held.
 **/
class Holding final : public v3d::editor::tool::Tool {
 public:
    void activate(const std::string& /* name */) override {
    }
    void deactivate(const std::string& /* name */) override {
    }
    void view(const boost::shared_ptr<v3d::editor::view::ViewPort>& view) override {
        shown = view;
    }
    bool takes(const glm::vec2& /* position */) const override {
        return true;
    }
    bool dragging() const noexcept override {
        return held;
    }
    void motion(const glm::vec2& /* position */) override {
    }
    void button(unsigned int /* button */, bool pressed, const glm::vec2& /* position */) override {
        held = pressed;
        presses += pressed ? 1 : 0;
    }

    boost::shared_ptr<v3d::editor::view::ViewPort> shown;
    bool held = false;
    int presses = 0;
};

/**
 * Two views side by side over a 200 by 100 region, a tool for them, and the pointer between.
 * The areas may be dragged down to 10 pixels, so a divider has room to move.
 **/
struct Panes final {
    Panes() :
        layout(boost::make_shared<v3d::editor::view::AreaLayout>(boost::make_shared<v3d::log::Logger>())),
        tool(boost::make_shared<Holding>()),
        tools(boost::make_shared<v3d::editor::tool::Router>()) {
        const boost::json::value parsed = boost::json::parse(R"({"name": "Pair", "root": {"split": "horizontal", "children": [)"
            R"({"editors": [{"kind": "viewport", "camera": "Front"}]}, {"editors": [{"kind": "viewport", "camera": "Top"}]}]}})");
        BOOST_REQUIRE(layout->load(parsed.as_object()));
        layout->metrics({ .gap = 3.0F, .reach = 4.0F, .least = 10.0F });
        areas = boost::make_shared<v3d::editor::view::AreaViews>(layout, [](const v3d::editor::view::AreaLayout::Editor& editor) {
            return boost::make_shared<v3d::editor::view::ViewPort>(std::to_string(editor.id), v3d::type::camera::Profile("Front"));
        });
        BOOST_REQUIRE(areas->sync());
        views = { areas->of(0), areas->of(1) };
        tools->add(tool);
        pointer = boost::make_shared<v3d::editor::control::Pointer>(nullptr, areas, tools,
            [this](const boost::shared_ptr<v3d::editor::view::ViewPort>& view) { moved.push_back(view); });
        pointer->resize(glm::vec4(0.0F, 0.0F, 200.0F, 100.0F));
    }

    boost::shared_ptr<v3d::editor::view::AreaLayout> layout;
    boost::shared_ptr<v3d::editor::view::AreaViews> areas;
    std::vector<boost::shared_ptr<v3d::editor::view::ViewPort>> views;
    boost::shared_ptr<Holding> tool;
    boost::shared_ptr<v3d::editor::tool::Router> tools;
    boost::shared_ptr<v3d::editor::control::Pointer> pointer;
    std::vector<boost::shared_ptr<v3d::editor::view::ViewPort>> moved;
};

/**
 * Two areas side by side over a 200 by 100 region, the first holding Front and Left. The ui's one
 * button stands for the first area's Left tab, from 10 to 50 across and 10 to 30 down. The areas
 * sit 3 pixels in, the first from 3 to 98 across and the second from 101 to 197.
 **/
struct Tabbed {
    Tabbed() :
        dispatcher(boost::make_shared<entt::dispatcher>()),
        ui(boost::make_shared<v3d::ui::Interface>(boost::make_shared<v3d::event::Router>(dispatcher), dispatcher,
            boost::make_shared<v3d::log::Logger>())),
        layout(boost::make_shared<v3d::editor::view::AreaLayout>(boost::make_shared<v3d::log::Logger>())),
        tool(boost::make_shared<Holding>()),
        tools(boost::make_shared<v3d::editor::tool::Router>()) {
        BOOST_REQUIRE(ui->load(boost::json::parse(R"({ "themes": [], "containers": [ { "name": "tabs", "visible": true, "components": [)"
            R"({ "type": "button", "name": "tab", "label": "Left", "context": "area", "command": "tab::choose" } ] } ] })").as_object()));
        const boost::shared_ptr<v3d::ui::Component> tab = ui->container("tabs")->get("tab");
        tab->position(glm::vec2(10.0F, 10.0F));
        tab->size(glm::vec2(40.0F, 20.0F));

        BOOST_REQUIRE(layout->load(boost::json::parse(R"({"root": {"split": "horizontal", "children": [)"
            R"({"editors": [{"kind": "viewport", "camera": "Front"}, {"kind": "viewport", "camera": "Left"}], "chosen": 1},)"
            R"({"editors": [{"kind": "viewport", "camera": "Top"}]}]}})").as_object()));
        layout->metrics({ .gap = 3.0F, .reach = 4.0F, .least = 10.0F });
        areas = boost::make_shared<v3d::editor::view::AreaViews>(layout, [](const v3d::editor::view::AreaLayout::Editor& editor) {
            return boost::make_shared<v3d::editor::view::ViewPort>(std::to_string(editor.id), v3d::type::camera::Profile("Front"));
        });
        BOOST_REQUIRE(areas->sync());
        tools->add(tool);
        pointer = boost::make_shared<v3d::editor::control::Pointer>(boost::make_shared<v3d::ui::input::Cursor>(ui, dispatcher), areas, tools,
            v3d::editor::control::Pointer::Moved());
        pointer->resize(glm::vec4(0.0F, 0.0F, 200.0F, 100.0F));
        moving = layout->areas()[0].editors[1].id;
        pointer->tabs([this](const glm::vec2& point) -> std::optional<std::size_t> {
            if (point.x >= 10.0F && point.x < 50.0F && point.y >= 10.0F && point.y < 30.0F) {
                return moving;
            }
            return std::nullopt;
        });
    }

    /**
     * Press on the tab, and move to a point.
     **/
    void drag(const glm::vec2& to) {
        pointer->motion(glm::vec2(30.0F, 20.0F));
        pointer->button(true);
        pointer->motion(to);
    }

    boost::shared_ptr<entt::dispatcher> dispatcher;
    boost::shared_ptr<v3d::ui::Interface> ui;
    boost::shared_ptr<v3d::editor::view::AreaLayout> layout;
    boost::shared_ptr<v3d::editor::view::AreaViews> areas;
    boost::shared_ptr<Holding> tool;
    boost::shared_ptr<v3d::editor::tool::Router> tools;
    boost::shared_ptr<v3d::editor::control::Pointer> pointer;
    std::size_t moving = 0;
};

BOOST_AUTO_TEST_SUITE(pointer_test)

/**
 * The view under the cursor becomes the active one and the tools are given it, but not while
 * a drag is under way, so a drag that crosses into the other view keeps its own.
 **/
BOOST_AUTO_TEST_CASE(the_active_view_follows_the_cursor_between_drags) {
    Panes panes;
    BOOST_CHECK(panes.pointer->activeView() == panes.views[0]);
    BOOST_CHECK(panes.tool->shown == panes.views[0]);

    panes.pointer->motion(glm::vec2(150.0F, 50.0F));
    BOOST_CHECK(panes.pointer->activeView() == panes.views[1]);
    BOOST_CHECK(panes.tool->shown == panes.views[1]);
    BOOST_REQUIRE_EQUAL(panes.moved.size(), 1U);

    panes.pointer->button(true);
    panes.pointer->motion(glm::vec2(20.0F, 50.0F));
    BOOST_CHECK(panes.pointer->activeView() == panes.views[1]);
    panes.pointer->button(false);
    BOOST_CHECK(!panes.tool->held);
    BOOST_CHECK_EQUAL(panes.tool->presses, 1);

    panes.pointer->motion(glm::vec2(20.0F, 50.0F));
    BOOST_CHECK(panes.pointer->activeView() == panes.views[0]);
    BOOST_CHECK_EQUAL(panes.moved.size(), 2U);
}

/**
 * A press on the divider between the views moves the divider, and no tool sees it or the
 * release that ends it. The areas sit 3 pixels in from the edge with a 3 pixel gap between
 * them, so the gap starts at 98 and the divider centred on 140 puts it at 138.
 **/
BOOST_AUTO_TEST_CASE(a_press_on_a_divider_reaches_no_tool) {
    Panes panes;
    panes.pointer->motion(glm::vec2(99.0F, 50.0F));
    panes.pointer->button(true);
    panes.pointer->motion(glm::vec2(140.0F, 50.0F));
    panes.pointer->button(false);

    BOOST_CHECK_EQUAL(panes.tool->presses, 0);
    BOOST_CHECK_EQUAL(panes.layout->areas()[0].region.z, 135.0F);
    BOOST_CHECK_EQUAL(panes.views[1]->region().x, 141.0F);
}

/**
 * A view's control that takes a press keeps it and its release from the dividers and the tools.
 * One that turns a press down leaves it to them.
 **/
BOOST_AUTO_TEST_CASE(a_views_control_takes_a_press_before_the_tools) {
    const Panes panes;
    std::vector<glm::vec2> offered;
    panes.pointer->controls([&offered](const glm::vec2& point) {
        offered.push_back(point);
        return point.x > 150.0F;
    });

    panes.pointer->motion(glm::vec2(160.0F, 20.0F));
    panes.pointer->button(true);
    panes.pointer->button(false);
    BOOST_CHECK_EQUAL(panes.tool->presses, 0);
    BOOST_CHECK(!panes.tool->held);

    panes.pointer->motion(glm::vec2(120.0F, 20.0F));
    panes.pointer->button(true);
    BOOST_CHECK_EQUAL(panes.tool->presses, 1);
    panes.pointer->button(false);
    BOOST_CHECK(!panes.tool->held);
    BOOST_REQUIRE_EQUAL(offered.size(), 2U);
    BOOST_CHECK(offered[0] == glm::vec2(160.0F, 20.0F));
}

/**
 * A press outside every area, as on a bar along the foot of the window, reaches no tool, and
 * neither does its release.
 **/
BOOST_AUTO_TEST_CASE(a_press_outside_the_areas_reaches_no_tool) {
    const Panes panes;
    panes.pointer->motion(glm::vec2(150.0F, 150.0F));
    panes.pointer->button(true);
    panes.pointer->button(false);
    BOOST_CHECK_EQUAL(panes.tool->presses, 0);

    panes.pointer->motion(glm::vec2(150.0F, 50.0F));
    panes.pointer->button(true);
    BOOST_CHECK_EQUAL(panes.tool->presses, 1);
    panes.pointer->button(false);
    BOOST_CHECK(!panes.tool->held);
}

/**
 * A press in an area that shows no view is the panel's: no tool sees it or its release, and the
 * panels have the cursor until they have been drawn after the release.
 **/
BOOST_AUTO_TEST_CASE(a_press_on_a_panel_reaches_no_tool) {
    const Panes panes;
    BOOST_REQUIRE(panes.layout->replace(1, boost::json::parse(R"({"kind": "outliner"})").as_object()));
    BOOST_REQUIRE(panes.areas->sync());
    panes.pointer->place();

    panes.pointer->motion(glm::vec2(50.0F, 50.0F));
    BOOST_CHECK(!panes.pointer->panels());
    panes.pointer->motion(glm::vec2(150.0F, 50.0F));
    BOOST_CHECK(panes.pointer->panels());

    panes.pointer->button(true);
    BOOST_CHECK_EQUAL(panes.tool->presses, 0);
    // the panel keeps the press when it is dragged out of its area
    panes.pointer->motion(glm::vec2(50.0F, 50.0F));
    BOOST_CHECK(panes.pointer->panels());
    panes.pointer->button(false);
    BOOST_CHECK(!panes.tool->held);
    BOOST_CHECK(panes.pointer->panels());
    panes.pointer->settle();
    BOOST_CHECK(!panes.pointer->panels());

    // near a divider the divider has the cursor, and not the panel beside it
    panes.pointer->motion(glm::vec2(102.0F, 50.0F));
    BOOST_CHECK(!panes.pointer->panels());
}

/**
 * With snapping, a dragged divider lands on a tenth of its split: the split runs 194 pixels
 * from 3, so a drag to 140 lands on its seventh tenth, centred at 138.8.
 **/
BOOST_AUTO_TEST_CASE(a_snapped_drag_lands_on_a_tenth) {
    const Panes panes;
    panes.pointer->motion(glm::vec2(99.0F, 50.0F));
    panes.pointer->button(true);
    panes.pointer->motion(glm::vec2(140.0F, 50.0F), true);
    panes.pointer->button(false);
    BOOST_CHECK_EQUAL(panes.layout->areas()[0].region.z, 134.0F);
}

/**
 * The second press of a double click on a divider evens it, and no tool sees either press or
 * either release.
 **/
BOOST_AUTO_TEST_CASE(a_double_click_evens_a_divider) {
    Panes panes;
    panes.pointer->motion(glm::vec2(99.0F, 50.0F));
    panes.pointer->button(true);
    panes.pointer->motion(glm::vec2(140.0F, 50.0F));
    panes.pointer->button(false);
    BOOST_REQUIRE_EQUAL(panes.layout->areas()[0].region.z, 135.0F);

    panes.pointer->button(true, 1);
    panes.pointer->button(false);
    panes.pointer->button(true, 2);
    panes.pointer->button(false);
    BOOST_CHECK_EQUAL(panes.layout->areas()[0].region.z, 95.0F);
    BOOST_CHECK_EQUAL(panes.views[1]->region().x, 101.0F);
    BOOST_CHECK_EQUAL(panes.tool->presses, 0);
}

/**
 * Cancelling puts a held divider back, and the release that follows reaches no tool.
 **/
BOOST_AUTO_TEST_CASE(a_cancelled_drag_puts_the_divider_back) {
    Panes panes;
    BOOST_CHECK(!panes.pointer->cancel());
    panes.pointer->motion(glm::vec2(99.0F, 50.0F));
    panes.pointer->button(true);
    panes.pointer->motion(glm::vec2(140.0F, 50.0F));
    BOOST_REQUIRE(panes.pointer->cancel());
    BOOST_CHECK_EQUAL(panes.layout->areas()[0].region.z, 95.0F);
    BOOST_CHECK_EQUAL(panes.views[1]->region().x, 101.0F);
    BOOST_CHECK(!panes.pointer->cancel());

    panes.pointer->motion(glm::vec2(150.0F, 50.0F));
    panes.pointer->button(false);
    BOOST_CHECK_EQUAL(panes.tool->presses, 0);
    BOOST_CHECK(!panes.tool->held);
}

/**
 * The divider lit is the one under the cursor or the one held, and none while a tool drags.
 **/
BOOST_AUTO_TEST_CASE(the_divider_under_the_cursor_is_lit) {
    const Panes panes;
    panes.pointer->motion(glm::vec2(99.0F, 50.0F));
    BOOST_REQUIRE(panes.pointer->lit().has_value());
    BOOST_CHECK_EQUAL(panes.pointer->lit().value_or(v3d::editor::view::AreaLayout::Divider()).region.x, 98.0F);
    panes.pointer->motion(glm::vec2(50.0F, 50.0F));
    BOOST_CHECK(!panes.pointer->lit().has_value());

    panes.pointer->motion(glm::vec2(99.0F, 50.0F));
    panes.pointer->button(true);
    panes.pointer->motion(glm::vec2(140.0F, 50.0F));
    BOOST_CHECK_EQUAL(panes.pointer->lit().value_or(v3d::editor::view::AreaLayout::Divider()).region.x, 138.0F);
    panes.pointer->button(false);

    // a tool's drag passes over the divider without lighting it
    panes.pointer->motion(glm::vec2(20.0F, 50.0F));
    panes.pointer->button(true);
    panes.pointer->motion(glm::vec2(139.0F, 50.0F));
    BOOST_CHECK(!panes.pointer->lit().has_value());
    panes.pointer->button(false);
}

/**
 * The active area follows the cursor, and a gap leaves it where it was.
 **/
BOOST_AUTO_TEST_CASE(the_active_area_follows_the_cursor) {
    const Panes panes;
    BOOST_CHECK_EQUAL(panes.pointer->activeArea(), 0U);
    panes.pointer->motion(glm::vec2(150.0F, 50.0F));
    BOOST_CHECK_EQUAL(panes.pointer->activeArea(), 1U);
    panes.pointer->motion(glm::vec2(99.0F, 50.0F));
    BOOST_CHECK_EQUAL(panes.pointer->activeArea(), 1U);
}

/**
 * A change to the layout that hides the active view makes the first view shown the active one,
 * and the tools are given it.
 **/
BOOST_AUTO_TEST_CASE(a_hidden_view_stops_being_the_active_one) {
    Panes panes;
    panes.pointer->motion(glm::vec2(150.0F, 50.0F));
    BOOST_REQUIRE(panes.pointer->activeView() == panes.views[1]);

    panes.layout->maximise(0);
    panes.pointer->place();
    BOOST_CHECK(panes.pointer->activeView() == panes.views[0]);
    BOOST_CHECK(panes.tool->shown == panes.views[0]);
    BOOST_CHECK(panes.moved.back() == panes.views[0]);
}

/**
 * A tab let go near another area's edge makes a new area on that edge, and the region it would
 * take is the half of the area on that side.
 **/
BOOST_FIXTURE_TEST_CASE(a_tab_dropped_on_an_edge_makes_an_area, Tabbed) {
    drag(glm::vec2(190.0F, 50.0F));
    const std::optional<v3d::editor::control::Pointer::Drop> landing = pointer->drop();
    BOOST_REQUIRE(landing.has_value());
    const v3d::editor::control::Pointer::Drop drop = landing.value_or(v3d::editor::control::Pointer::Drop());
    BOOST_CHECK_EQUAL(drop.area, 1U);
    BOOST_CHECK(drop.edge == v3d::editor::view::AreaLayout::Edge::Right);
    BOOST_CHECK(drop.region == glm::vec4(149.0F, 3.0F, 48.0F, 94.0F));

    pointer->button(false);
    BOOST_REQUIRE_EQUAL(layout->areas().size(), 3U);
    BOOST_CHECK_EQUAL(layout->areas()[0].editors.size(), 1U);
    BOOST_CHECK_EQUAL(layout->areas()[2].editor().id, moving);
    BOOST_CHECK_GT(layout->areas()[2].region.x, layout->areas()[1].region.x);
    BOOST_CHECK(!pointer->drop().has_value());
    BOOST_CHECK_EQUAL(tool->presses, 0);
}

/**
 * A tab let go in the middle of another area joins that area's tabs and is shown there.
 **/
BOOST_FIXTURE_TEST_CASE(a_tab_dropped_in_an_area_joins_its_tabs, Tabbed) {
    drag(glm::vec2(149.0F, 50.0F));
    const std::optional<v3d::editor::control::Pointer::Drop> landing = pointer->drop();
    BOOST_REQUIRE(landing.has_value());
    BOOST_CHECK(!landing.value_or(v3d::editor::control::Pointer::Drop()).edge.has_value());

    pointer->button(false);
    BOOST_REQUIRE_EQUAL(layout->areas().size(), 2U);
    BOOST_CHECK_EQUAL(layout->areas()[1].editors.size(), 2U);
    BOOST_CHECK_EQUAL(layout->areas()[1].editor().id, moving);
}

/**
 * A press on a tab that hardly moves is a click and moves nothing, and a tab let go in the
 * middle of its own area lands nowhere.
 **/
BOOST_FIXTURE_TEST_CASE(a_tab_that_hardly_moves_stays, Tabbed) {
    drag(glm::vec2(33.0F, 22.0F));
    BOOST_CHECK(!pointer->drop().has_value());
    pointer->button(false);
    BOOST_CHECK_EQUAL(layout->areas()[0].editors.size(), 2U);

    drag(glm::vec2(50.0F, 60.0F));
    BOOST_CHECK(!pointer->drop().has_value());
    pointer->button(false);
    BOOST_CHECK_EQUAL(layout->areas().size(), 2U);
    BOOST_CHECK_EQUAL(layout->areas()[0].editors.size(), 2U);
    BOOST_CHECK_EQUAL(tool->presses, 0);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

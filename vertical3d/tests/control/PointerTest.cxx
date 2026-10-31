/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/control/Pointer.h>
#include <vertical3d/src/tool/Tool.h>
#include <vertical3d/src/tool/ToolRouter.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/ViewPort.h>

#include <string>
#include <vector>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

/**
 * A tool that takes every press, and remembers the view it was given and whether it is held.
 **/
class Holding final : public v3d::editor::Tool {
 public:
    void activate(const std::string& /* name */) override {
    }
    void deactivate(const std::string& /* name */) override {
    }
    void view(const boost::shared_ptr<v3d::editor::ViewPort>& view) override {
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

    boost::shared_ptr<v3d::editor::ViewPort> shown;
    bool held = false;
    int presses = 0;
};

/**
 * Two views side by side over a 200 by 100 region, a tool for them, and the pointer between.
 * The areas may be dragged down to 10 pixels, so a divider has room to move.
 **/
struct Panes final {
    Panes() :
        layout(boost::make_shared<v3d::editor::AreaLayout>(boost::make_shared<v3d::log::Logger>())),
        tool(boost::make_shared<Holding>()),
        tools(boost::make_shared<v3d::editor::ToolRouter>()) {
        const boost::json::value parsed = boost::json::parse(R"({"name": "Pair", "root": {"split": "horizontal", "children": [)"
            R"({"editors": [{"kind": "viewport", "camera": "Front"}]}, {"editors": [{"kind": "viewport", "camera": "Top"}]}]}})");
        BOOST_REQUIRE(layout->load(parsed.as_object()));
        layout->metrics({ .gap = 3.0F, .reach = 4.0F, .least = 10.0F });
        areas = boost::make_shared<v3d::editor::AreaViews>(layout, [](const v3d::editor::AreaLayout::Editor& editor) {
            return boost::make_shared<v3d::editor::ViewPort>(std::to_string(editor.id), v3d::type::camera::Profile("Front"));
        });
        BOOST_REQUIRE(areas->sync());
        views = { areas->of(0), areas->of(1) };
        tools->add(tool);
        pointer = boost::make_shared<v3d::editor::Pointer>(nullptr, areas, tools,
            [this](const boost::shared_ptr<v3d::editor::ViewPort>& view) { moved.push_back(view); });
        pointer->resize(glm::vec4(0.0F, 0.0F, 200.0F, 100.0F));
    }

    boost::shared_ptr<v3d::editor::AreaLayout> layout;
    boost::shared_ptr<v3d::editor::AreaViews> areas;
    std::vector<boost::shared_ptr<v3d::editor::ViewPort>> views;
    boost::shared_ptr<Holding> tool;
    boost::shared_ptr<v3d::editor::ToolRouter> tools;
    boost::shared_ptr<v3d::editor::Pointer> pointer;
    std::vector<boost::shared_ptr<v3d::editor::ViewPort>> moved;
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

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

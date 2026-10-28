/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/kind/Json.h>
#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/control/Pointer.h>
#include <vertical3d/src/tool/Tool.h>
#include <vertical3d/src/tool/ToolRouter.h>
#include <vertical3d/src/view/ViewLayout.h>
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
 * Two views side by side over a 200 by 100 area, a tool for them, and the pointer between.
 **/
struct Panes final {
    Panes() :
        layout(boost::make_shared<v3d::editor::ViewLayout>(boost::make_shared<v3d::log::Logger>())),
        tool(boost::make_shared<Holding>()),
        tools(boost::make_shared<v3d::editor::ToolRouter>()) {
        const boost::json::value parsed = boost::json::parse(
            R"({"layout": {"name": "Pair", "root": {"split": "horizontal", "children": [{"camera": "Front"}, {"camera": "Top"}]}}})");
        BOOST_REQUIRE(layout->load(boost::make_shared<v3d::asset::kind::Json>("layout", parsed.as_object())));
        for (const char* name : { "left", "right" }) {
            views.push_back(boost::make_shared<v3d::editor::ViewPort>(name, v3d::type::camera::Profile("Front")));
        }
        tools->add(tool);
        pointer = boost::make_shared<v3d::editor::Pointer>(nullptr, layout, views, tools,
            [this](const boost::shared_ptr<v3d::editor::ViewPort>& view) { moved.push_back(view); });
        pointer->resize(glm::vec4(0.0F, 0.0F, 200.0F, 100.0F));
    }

    boost::shared_ptr<v3d::editor::ViewLayout> layout;
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
 * release that ends it.
 **/
BOOST_AUTO_TEST_CASE(a_press_on_a_divider_reaches_no_tool) {
    Panes panes;
    panes.pointer->motion(glm::vec2(100.0F, 50.0F));
    panes.pointer->button(true);
    panes.pointer->motion(glm::vec2(140.0F, 50.0F));
    panes.pointer->button(false);

    BOOST_CHECK_EQUAL(panes.tool->presses, 0);
    BOOST_CHECK_CLOSE(panes.layout->views()[0].region.z, 140.0F, 0.001F);
    BOOST_CHECK_CLOSE(panes.views[1]->region().x, 140.0F, 0.001F);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

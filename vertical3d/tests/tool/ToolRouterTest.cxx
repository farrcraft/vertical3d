/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vertical3d/src/tool/Tool.h>
#include <vertical3d/src/tool/ToolRouter.h>

#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>

namespace {

/**
 * A tool that takes a press when told to, and writes down what it heard.
 **/
class Recording final : public v3d::editor::Tool {
 public:
    explicit Recording(bool takes) : taking(takes) {
    }

    void activate(const std::string& /* name */) override {
    }
    void deactivate(const std::string& /* name */) override {
    }
    void view(const boost::shared_ptr<v3d::editor::ViewPort>& /* view */) override {
        heard.emplace_back("view");
    }
    bool takes(const glm::vec2& /* position */) const override {
        return taking;
    }
    bool dragging() const noexcept override {
        return held;
    }
    void motion(const glm::vec2& /* position */) override {
        heard.emplace_back("motion");
    }
    void button(unsigned int /* button */, bool pressed, const glm::vec2& /* position */) override {
        held = pressed;
        heard.emplace_back(pressed ? "press" : "release");
    }

    std::vector<std::string> heard;
    bool taking;
    bool held = false;
};

BOOST_AUTO_TEST_SUITE(tool_router_test)

/**
 * A press goes to the first tool that takes it, in the order the tools were added, and the
 * release goes to that tool alone. The tools before it and after it hear neither.
 **/
BOOST_AUTO_TEST_CASE(a_press_goes_to_the_first_tool_that_takes_it) {
    const boost::shared_ptr<Recording> declines = boost::make_shared<Recording>(false);
    const boost::shared_ptr<Recording> takes = boost::make_shared<Recording>(true);
    const boost::shared_ptr<Recording> after = boost::make_shared<Recording>(true);
    v3d::editor::ToolRouter router;
    router.add(declines);
    router.add(takes);
    router.add(after);

    BOOST_CHECK(router.button(true, glm::vec2(1.0F, 1.0F)));
    BOOST_CHECK(router.active() == takes);
    BOOST_CHECK(router.dragging());
    BOOST_CHECK(router.button(false, glm::vec2(2.0F, 2.0F)));
    BOOST_CHECK(!router.active());
    BOOST_CHECK(!router.dragging());

    BOOST_CHECK((takes->heard == std::vector<std::string>{ "press", "release" }));
    BOOST_CHECK(declines->heard.empty());
    BOOST_CHECK(after->heard.empty());
}

/**
 * With no tool to take it, a press goes nowhere, and so does its release.
 **/
BOOST_AUTO_TEST_CASE(a_press_no_tool_takes_goes_nowhere) {
    const boost::shared_ptr<Recording> declines = boost::make_shared<Recording>(false);
    v3d::editor::ToolRouter router;
    router.add(declines);

    BOOST_CHECK(!router.button(true, glm::vec2(1.0F, 1.0F)));
    BOOST_CHECK(!router.button(false, glm::vec2(1.0F, 1.0F)));
    BOOST_CHECK(declines->heard.empty());
}

/**
 * Every tool hears every motion and the view, whichever has the gesture. A second press while
 * a gesture is under way ends that gesture before the new one starts.
 **/
BOOST_AUTO_TEST_CASE(every_tool_hears_the_motion_and_the_view) {
    const boost::shared_ptr<Recording> first = boost::make_shared<Recording>(true);
    const boost::shared_ptr<Recording> second = boost::make_shared<Recording>(false);
    v3d::editor::ToolRouter router;
    router.add(first);
    router.add(second);

    router.view(nullptr);
    router.button(true, glm::vec2(0.0F, 0.0F));
    router.motion(glm::vec2(3.0F, 3.0F));
    router.button(true, glm::vec2(4.0F, 4.0F));

    BOOST_CHECK((first->heard == std::vector<std::string>{ "view", "press", "motion", "release", "press" }));
    BOOST_CHECK((second->heard == std::vector<std::string>{ "view", "motion" }));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

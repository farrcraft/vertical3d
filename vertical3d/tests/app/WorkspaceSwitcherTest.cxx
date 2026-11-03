/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/app/WorkspaceSwitcher.h>

#include <vector>

#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

namespace {

/**
 * A switcher over a context with no project, recording the windows it asks to be opened and
 * keeping the windows it is told are open.
 **/
struct Switched {
    Switched() :
        switcher(&context) {
        context.logger = boost::make_shared<v3d::log::Logger>();
        context.keptWindows = [this]() { return open; };
        context.reopen = [this](const boost::json::array& windows) { reopened.push_back(windows); };
    }

    /**
     * @return a window of a width holding a render view, as a workspace keeps it
     **/
    static boost::json::object window(int width) {
        return boost::json::object{ { "width", width }, { "height", 200 },
            { "layout", boost::json::parse(R"({"root": {"editors": [{"kind": "render-view"}]}})") } };
    }

    v3d::editor::app::Context context;
    v3d::editor::app::WorkspaceSwitcher switcher;
    boost::json::array open;
    std::vector<boost::json::array> reopened;
};

BOOST_AUTO_TEST_SUITE(workspace_switcher_test)

/**
 * A workspace that keeps a window opens it when it is shown, and one that keeps none closes
 * those open.
 **/
BOOST_AUTO_TEST_CASE(a_workspace_opens_the_windows_it_keeps) {
    Switched switched;
    boost::json::object first = boost::json::parse(R"({"name": "First", "root": {"editors": [{"kind": "outliner"}]}})").as_object();
    first["windows"] = boost::json::array{ Switched::window(300) };
    const boost::json::object second = boost::json::parse(R"({"name": "Second", "root": {"editors": [{"kind": "outliner"}]}})").as_object();
    const boost::json::object shipped{ { "workspaces", boost::json::array{ first, second } } };

    BOOST_REQUIRE(switched.switcher.start(&shipped));
    BOOST_REQUIRE_EQUAL(switched.reopened.size(), 1U);
    BOOST_REQUIRE_EQUAL(switched.reopened[0].size(), 1U);
    BOOST_CHECK_EQUAL(switched.reopened[0][0].as_object().at("width").as_int64(), 300);

    BOOST_REQUIRE(switched.switcher.show("Second"));
    BOOST_REQUIRE_EQUAL(switched.reopened.size(), 2U);
    BOOST_CHECK(switched.reopened[1].empty());
}

/**
 * The windows open when a workspace is left are kept with its arrangement, and come back when
 * it is shown again.
 **/
BOOST_AUTO_TEST_CASE(a_workspace_keeps_its_open_windows) {
    Switched switched;
    const boost::json::object shipped = boost::json::parse(R"({"workspaces": [
        {"name": "First", "root": {"editors": [{"kind": "outliner"}]}},
        {"name": "Second", "root": {"editors": [{"kind": "outliner"}]}}]})").as_object();
    BOOST_REQUIRE(switched.switcher.start(&shipped));

    switched.open = boost::json::array{ Switched::window(500) };
    switched.switcher.record();
    switched.open.clear();
    BOOST_REQUIRE(switched.switcher.show("Second"));
    BOOST_REQUIRE(switched.switcher.show("First"));
    BOOST_REQUIRE(!switched.reopened.empty());
    const boost::json::array& last = switched.reopened.back();
    BOOST_REQUIRE_EQUAL(last.size(), 1U);
    BOOST_CHECK_EQUAL(last[0].as_object().at("width").as_int64(), 500);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/view/AreaLayout.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

using v3d::editor::view::AreaLayout;

/**
 * A context over two areas side by side: a viewport, and an area holding an outliner and a
 * properties editor as tabs, the outliner shown. It counts the changes to the areas, and opens a
 * window only when one is allowed, keeping each kind it was asked to show.
 **/
struct Shown {
    Shown() :
        layout(boost::make_shared<AreaLayout>(boost::make_shared<v3d::log::Logger>())) {
        BOOST_REQUIRE(layout->load(boost::json::parse(R"({"root": {"split": "horizontal", "children": [)"
            R"({"editors": [{"kind": "viewport", "camera": "Front"}]},)"
            R"({"editors": [{"kind": "outliner"}, {"kind": "properties"}]}]}})").as_object()));
        context.layout = layout;
        context.areasChanged = [this]() { changes++; };
        context.window = [this](std::string_view kind) {
            asked.emplace_back(kind);
            return windows;
        };
    }

    /**
     * @return the kinds of an area's editors, in its tabs' order
     **/
    std::vector<std::string> kinds(std::size_t area) const {
        std::vector<std::string> made;
        for (const AreaLayout::Editor& editor : layout->areas()[area].editors) {
            made.push_back(editor.kind);
        }
        return made;
    }

    boost::shared_ptr<AreaLayout> layout;
    v3d::editor::app::Context context;
    // what the context's callbacks record, which a case reads from a fixture it never changes
    mutable std::vector<std::string> asked;
    bool windows = true;
    mutable int changes = 0;
};

BOOST_AUTO_TEST_SUITE(context_test)

/**
 * An editor an area holds is chosen there, and no window is opened for it.
 **/
BOOST_AUTO_TEST_CASE(an_editor_an_area_holds_is_chosen) {
    const Shown shown;
    shown.context.show("properties", true);
    BOOST_CHECK_EQUAL(shown.layout->areas()[1].chosen, 1U);
    BOOST_CHECK(shown.asked.empty());
    BOOST_CHECK_EQUAL(shown.changes, 1);
}

/**
 * An editor no area holds, asked for in a window, is shown in one, and no area changes.
 **/
BOOST_AUTO_TEST_CASE(an_editor_no_area_holds_opens_a_window) {
    const Shown shown;
    shown.context.show("render-view", true);
    BOOST_CHECK(shown.asked == std::vector<std::string>({ "render-view" }));
    BOOST_CHECK(shown.kinds(0) == std::vector<std::string>({ "viewport" }));
    BOOST_CHECK_EQUAL(shown.changes, 0);
}

/**
 * When no window can be opened, the editor is added as a tab of the active area instead.
 **/
BOOST_AUTO_TEST_CASE(an_editor_with_no_window_becomes_a_tab) {
    Shown shown;
    shown.windows = false;
    shown.context.show("render-view", true);
    BOOST_CHECK(shown.kinds(0) == std::vector<std::string>({ "viewport", "render-view" }));
    BOOST_CHECK_EQUAL(shown.changes, 1);
}

/**
 * An editor not asked for in a window is added as a tab, and no window is asked for.
 **/
BOOST_AUTO_TEST_CASE(an_editor_not_windowed_becomes_a_tab) {
    const Shown shown;
    shown.context.show("render-settings");
    BOOST_CHECK(shown.asked.empty());
    BOOST_CHECK(shown.kinds(0) == std::vector<std::string>({ "viewport", "render-settings" }));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

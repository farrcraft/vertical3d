/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Event.h>
#include <api/event/Router.h>
#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Component.h>
#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Label.h>
#include <api/ui/paint/ComponentRenderer.h>
#include <vertical3d/src/control/TopBar.h>

#include <string>
#include <string_view>
#include <variant>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>
#include <boost/pointer_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::control::TopBar;

/**
 * A ui holding the editor's own container and a dialog's after it, with text 10 pixels a
 * character. The bar goes in a band from 300 to 800 across, 28 pixels tall.
 **/
struct Bar {
    Bar() :
        dispatcher(boost::make_shared<entt::dispatcher>()),
        ui(boost::make_shared<v3d::ui::Interface>(boost::make_shared<v3d::event::Router>(dispatcher), dispatcher,
            boost::make_shared<v3d::log::Logger>())),
        bar(ui, "chooser", [](std::string_view text) { return static_cast<float>(text.size()) * 10.0F; }),
        renderer([](std::string_view text) { return static_cast<float>(text.size()) * 10.0F; },
            [](std::string_view, const glm::vec2&, const glm::vec4&) {}) {
        BOOST_REQUIRE(ui->load(boost::json::parse(R"({ "themes": [], "containers": [
            { "name": "editor", "visible": true, "components": [] },
            { "name": "chooser", "visible": false, "components": [] } ] })").as_object()));
        canvas.resize(glm::uvec2(800, 600));
    }

    void sync() {
        bar.sync(state, band);
        canvas.clear();
        for (const boost::shared_ptr<v3d::ui::Container>& each : ui->containers()) {
            renderer.draw(&canvas, *each);
        }
    }

    boost::shared_ptr<v3d::ui::Component> get(const std::string& name) const {
        return ui->container(std::string(TopBar::NAME))->get(name);
    }

    boost::shared_ptr<v3d::ui::component::Button> tab(int index) const {
        return boost::dynamic_pointer_cast<v3d::ui::component::Button>(get("workspace-" + std::to_string(index)));
    }

    boost::shared_ptr<entt::dispatcher> dispatcher;
    boost::shared_ptr<v3d::ui::Interface> ui;
    TopBar bar;
    v3d::render::realtime::canvas::Canvas canvas;
    v3d::ui::paint::ComponentRenderer renderer;
    TopBar::State state{ .workspaces = { "Modelling", "Perspective" }, .shown = "Perspective", .project = "house", .dirty = false };
    glm::vec4 band{ 300.0F, 0.0F, 500.0F, 28.0F };
};

BOOST_AUTO_TEST_SUITE(top_bar_test)

/**
 * The bar goes between the editor's container and the dialogs. It has a tab for each workspace
 * that sends workspace::show with the workspace's name, and the one shown is checked.
 **/
BOOST_FIXTURE_TEST_CASE(a_tab_for_each_workspace, Bar) {
    sync();
    BOOST_REQUIRE_EQUAL(ui->containers().size(), 3U);
    BOOST_CHECK_EQUAL(ui->containers()[1]->name(), TopBar::NAME);

    BOOST_REQUIRE(tab(0) && tab(1));
    BOOST_CHECK_EQUAL(tab(0)->label(), "Modelling");
    BOOST_CHECK_EQUAL(tab(0)->event().str(), "workspace::show");
    const boost::optional<v3d::event::EventData> data = tab(1)->event().data();
    BOOST_REQUIRE(data && std::holds_alternative<std::string>(*data));
    BOOST_CHECK_EQUAL(std::get<std::string>(*data), "Perspective");
    BOOST_CHECK(!tab(0)->checked());
    BOOST_CHECK(tab(1)->checked());

    state.shown = "Modelling";
    sync();
    BOOST_CHECK(tab(0)->checked());
    BOOST_CHECK(!tab(1)->checked());
}

/**
 * The search button sits at the right end of the band, with the project's name before it, which
 * carries a mark while there are unsaved changes. The tabs start at the band's left.
 **/
BOOST_FIXTURE_TEST_CASE(the_name_and_the_search_sit_at_the_right, Bar) {
    state.dirty = true;
    sync();
    const boost::shared_ptr<v3d::ui::component::Button> search = boost::dynamic_pointer_cast<v3d::ui::component::Button>(get("search"));
    BOOST_REQUIRE(search);
    BOOST_CHECK_EQUAL(search->event().str(), "search::open");
    BOOST_CHECK_EQUAL(search->position().x + search->size().x, 800.0F);

    const boost::shared_ptr<v3d::ui::component::Label> project = boost::dynamic_pointer_cast<v3d::ui::component::Label>(get("project"));
    BOOST_REQUIRE(project);
    BOOST_CHECK_EQUAL(project->text(), "house *");
    BOOST_CHECK_EQUAL(project->position().x + project->size().x, search->position().x);
    BOOST_CHECK_GE(get("workspaces")->position().x, 300.0F);

    state.dirty = false;
    sync();
    BOOST_CHECK_EQUAL(project->text(), "house");
}

/**
 * A change to the workspaces builds the bar again with a tab for each.
 **/
BOOST_FIXTURE_TEST_CASE(new_workspaces_build_the_bar_again, Bar) {
    sync();
    const boost::shared_ptr<v3d::ui::Container> before = ui->container(std::string(TopBar::NAME));
    sync();
    BOOST_CHECK(ui->container(std::string(TopBar::NAME)) == before);

    state.workspaces.emplace_back("Animation");
    sync();
    BOOST_CHECK(ui->container(std::string(TopBar::NAME)) != before);
    BOOST_REQUIRE(tab(2));
    BOOST_CHECK_EQUAL(tab(2)->label(), "Animation");
    BOOST_CHECK_EQUAL(ui->containers().size(), 3U);
}

BOOST_AUTO_TEST_CASE(a_title_marks_unsaved_changes) {
    BOOST_CHECK_EQUAL(TopBar::title("house", false), "house");
    BOOST_CHECK_EQUAL(TopBar::title("house", true), "house *");
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

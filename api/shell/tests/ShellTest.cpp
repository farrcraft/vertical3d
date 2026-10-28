/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/Engine.h>
#include <api/engine/Feature.h>
#include <api/engine/Statistics.h>
#include <api/event/Context.h>
#include <api/event/Event.h>
#include <api/event/State.h>
#include <api/shell/Shell.h>
#include <api/ui/shell/GameMenu.h>
#include <api/ui/shell/Screen.h>
#include <api/ui/shell/StatisticsOverlay.h>

#include <algorithm>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * An app with the config feature and nothing else, so it starts with no window. A case reaches
 * the engine's measure() through it, as an app does.
 **/
class TestEngine final : public v3d::engine::Engine {
 public:
    explicit TestEngine(const std::string& path) : Engine(path) {
    }

    using Engine::measure;

 protected:
    v3d::engine::Features features() const override {
        return v3d::engine::Feature::Config;
    }
};

/**
 * The fixture whose ui document names the game menu under GameMenu's default names.
 **/
const char* const game = "fixtures/game/";

/**
 * A press of a command, as a binding sends it.
 **/
v3d::event::Event press(const std::string& context, const std::string& name) {
    v3d::event::Event event(name, boost::make_shared<v3d::event::Context>(context));
    event.state(v3d::event::State::Pressed);
    return event;
}

/**
 * The commands are declared as the shell is built, before the engine reads the bindings that
 * send them. A part not asked for declares nothing.
 **/
BOOST_AUTO_TEST_CASE(shell_declares_the_commands_it_is_asked_for) {
    TestEngine asked(game);
    const v3d::shell::Shell shell(&asked, { .menu = true, .statistics = "game::statistics" });
    BOOST_TEST(asked.commands().has("ui::showGameMenu"));
    BOOST_TEST(asked.commands().has("game::statistics"));

    TestEngine bare(game);
    const v3d::shell::Shell none(&bare, { .menu = false, .statistics = "" });
    BOOST_TEST(!bare.commands().has("ui::showGameMenu"));
    BOOST_TEST(!bare.commands().has("game::statistics"));
}

/**
 * start() loads the ui document and puts up the menu it names. Opening the menu suspends the
 * game through the callback start() was given, and closing it resumes the game.
 **/
BOOST_AUTO_TEST_CASE(shell_puts_up_the_menu_the_ui_names) {
    TestEngine engine(game);
    v3d::shell::Shell shell(&engine, { .menu = true, .statistics = "" });
    BOOST_REQUIRE(engine.initialize());
    std::vector<bool> suspended;
    BOOST_REQUIRE(shell.start([&suspended](bool on) { suspended.push_back(on); }));
    BOOST_REQUIRE(shell.ui());
    BOOST_REQUIRE(shell.ui()->container(v3d::ui::shell::GameMenu::defaultContainer));
    BOOST_REQUIRE(shell.menu());

    BOOST_TEST(engine.commands().invoke(press("ui", "showGameMenu")));
    BOOST_TEST(shell.menu()->visible());
    shell.menu()->toggle();
    BOOST_TEST(!shell.menu()->visible());
    BOOST_TEST(suspended == std::vector<bool>({ true, false }), boost::test_tools::per_element());
}

/**
 * The statistics command shows and hides the screen's overlay. Before present() there is no
 * screen, and the command does nothing.
 **/
BOOST_AUTO_TEST_CASE(shell_shows_the_statistics_on_its_command) {
    TestEngine engine(game);
    v3d::shell::Shell shell(&engine, { .menu = false, .statistics = "game::statistics" });
    BOOST_REQUIRE(engine.initialize());
    BOOST_REQUIRE(shell.start());
    BOOST_TEST(engine.commands().invoke(press("game", "statistics")));
    BOOST_TEST(!shell.menu());

    shell.present(nullptr, v3d::ui::shell::Screen::Options());
    BOOST_REQUIRE(shell.screen());
    BOOST_REQUIRE(shell.screen()->statistics());
    const bool before = shell.screen()->statistics()->visible();
    BOOST_TEST(engine.commands().invoke(press("game", "statistics")));
    BOOST_TEST(shell.screen()->statistics()->visible() != before);

    shell.release();
    BOOST_TEST(!shell.screen());
    BOOST_TEST(!shell.ui());
    BOOST_TEST(engine.commands().invoke(press("game", "statistics")));
}

/**
 * A sample carries a span for each scope the app measured, so the overlay shows an app's own
 * timings without the app copying them in.
 **/
BOOST_AUTO_TEST_CASE(shell_samples_what_the_app_measured) {
    TestEngine engine(game);
    const v3d::shell::Shell shell(&engine, { .menu = false, .statistics = "" });
    BOOST_REQUIRE(engine.initialize());
    {
        const v3d::engine::Statistics::Scope scope = engine.measure("chunks");
    }

    const v3d::ui::shell::StatisticsOverlay::Sample sample = shell.sample();
    BOOST_TEST(std::ranges::any_of(sample.spans,
        [](const v3d::ui::shell::StatisticsOverlay::Sample::Span& span) { return span.name == "chunks"; }));
}

};  // namespace

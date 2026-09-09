/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Json.h>
#include <api/ui/Engine.h>
#include <api/ui/GameMenu.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * A menu of two items inside a submenu, which is what a toggle has to walk back out of one
 * level at a time.
 **/
const char* const document = R"({
  "themes": [ { "name": "default" } ],
  "containers": [
    {
      "name": "game-menu",
      "visible": false,
      "components": [
        {
          "name": "main-menu",
          "type": "menu",
          "items": [
            { "label": "Resume", "command": "showGameMenu", "context": "ui", "type": "action" },
            {
              "label": "Options",
              "type": "submenu",
              "items": [
                { "label": "Back", "command": "showGameMenu", "context": "ui", "type": "action" }
              ]
            }
          ]
        }
      ]
    }
  ]
})";

/**
 * A menu whose first item captures a key and whose second is an ordinary action, which is
 * the shape pong's Options screen has.
 **/
const char* const bindings = R"({
  "themes": [ { "name": "default" } ],
  "containers": [
    {
      "name": "game-menu",
      "visible": false,
      "components": [
        {
          "name": "main-menu",
          "type": "menu",
          "items": [
            { "label": "Player 1 Up: ", "command": "setLeftPaddleUpKey", "context": "ui", "type": "key_input" },
            { "label": "Resume", "command": "showGameMenu", "context": "ui", "type": "action" }
          ]
        }
      ]
    }
  ]
})";

/**
 * A ui engine over a document written inline, which is what a config file amounts to by
 * the time it reaches the loader.
 **/
boost::shared_ptr<v3d::ui::Engine> load(const std::string& config) {
    boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    boost::shared_ptr<v3d::ui::Engine> ui = boost::make_shared<v3d::ui::Engine>(
        boost::make_shared<v3d::event::Engine>(dispatcher),
        dispatcher,
        boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(ui->load(boost::make_shared<v3d::asset::Json>(
        "vgui", v3d::asset::Type::JsonDocument, boost::json::parse(config).as_object())));
    return ui;
}

/**
 * The menu's active item, by label, which is what navigation moves.
 **/
std::string active(const boost::shared_ptr<v3d::ui::Engine>& ui) {
    boost::shared_ptr<v3d::ui::component::Menu> menu =
        boost::dynamic_pointer_cast<v3d::ui::component::Menu>(ui->container("game-menu")->get("main-menu"));
    boost::shared_ptr<v3d::ui::component::Menu> level = menu->level();
    if (!level || !level->active()) {
        return std::string();
    }
    return std::string(level->active()->label());
}

};  // namespace

BOOST_AUTO_TEST_SUITE(game_menu_test)

/**
 * The container is what is shown and hidden, and the game is suspended for exactly as long
 * as it is up.
 **/
BOOST_AUTO_TEST_CASE(a_toggle_shows_the_container_and_suspends_the_game) {
    boost::shared_ptr<v3d::ui::Engine> ui = load(document);
    std::vector<bool> suspended;
    v3d::ui::GameMenu menu(ui, [&suspended](bool state) { suspended.push_back(state); });

    BOOST_TEST(!menu.visible());

    menu.toggle();
    BOOST_TEST(menu.visible());
    BOOST_TEST(ui->container("game-menu")->visible());
    BOOST_REQUIRE_EQUAL(suspended.size(), 1U);
    BOOST_TEST(suspended[0]);

    menu.toggle();
    BOOST_TEST(!menu.visible());
    BOOST_TEST(!ui->container("game-menu")->visible());
    BOOST_REQUIRE_EQUAL(suspended.size(), 2U);
    BOOST_TEST(!suspended[1]);
}

/**
 * Going back up out of a submenu leaves the menu open - it is only closing the top level
 * that resumes the game, so a toggle inside a submenu is not a close.
 **/
BOOST_AUTO_TEST_CASE(a_toggle_inside_a_submenu_only_goes_up_a_level) {
    boost::shared_ptr<v3d::ui::Engine> ui = load(document);
    std::vector<bool> suspended;
    v3d::ui::GameMenu menu(ui, [&suspended](bool state) { suspended.push_back(state); });

    menu.toggle();
    BOOST_TEST(menu.navigate("menuNext"));
    BOOST_TEST(active(ui) == "Options");
    // descend into the submenu the way activating it does
    BOOST_TEST(menu.navigate("selectMenu"));
    BOOST_TEST(active(ui) == "Back");

    menu.toggle();
    BOOST_TEST(menu.visible());
    BOOST_TEST(active(ui) == "Options");
    // still one suspend, from putting the menu up
    BOOST_REQUIRE_EQUAL(suspended.size(), 1U);

    menu.toggle();
    BOOST_TEST(!menu.visible());
    BOOST_REQUIRE_EQUAL(suspended.size(), 2U);
    BOOST_TEST(!suspended[1]);
}

/**
 * Navigation moves the active item, and wraps at each end.
 **/
BOOST_AUTO_TEST_CASE(navigation_moves_the_active_item) {
    boost::shared_ptr<v3d::ui::Engine> ui = load(document);
    v3d::ui::GameMenu menu(ui, v3d::ui::GameMenu::Suspend());
    menu.toggle();

    BOOST_TEST(active(ui) == "Resume");
    BOOST_TEST(menu.navigate("menuNext"));
    BOOST_TEST(active(ui) == "Options");
    BOOST_TEST(menu.navigate("menuPrevious"));
    BOOST_TEST(active(ui) == "Resume");
}

/**
 * A command that is not one of the three is left for the app to act on itself.
 **/
BOOST_AUTO_TEST_CASE(an_unrelated_command_is_not_taken) {
    boost::shared_ptr<v3d::ui::Engine> ui = load(document);
    v3d::ui::GameMenu menu(ui, v3d::ui::GameMenu::Suspend());
    menu.toggle();

    BOOST_TEST(!menu.navigate("moveForward"));
}

/**
 * Nothing is navigated while the menu is down: the same key that moves a menu item plays
 * the game when there is no menu over it.
 **/
BOOST_AUTO_TEST_CASE(nothing_is_navigated_while_the_menu_is_down) {
    boost::shared_ptr<v3d::ui::Engine> ui = load(document);
    v3d::ui::GameMenu menu(ui, v3d::ui::GameMenu::Suspend());

    BOOST_TEST(!menu.navigate("menuNext"));
    BOOST_TEST(active(ui) == "Resume");
}

/**
 * A config that names no such container leaves every call doing nothing, rather than
 * failing - a game whose ui carries no menu still runs, without one.
 **/
BOOST_AUTO_TEST_CASE(a_missing_container_leaves_the_menu_inert) {
    boost::shared_ptr<v3d::ui::Engine> ui = load(R"({"themes": [], "containers": []})");
    std::vector<bool> suspended;
    v3d::ui::GameMenu menu(ui, [&suspended](bool state) { suspended.push_back(state); });

    menu.toggle();
    BOOST_TEST(!menu.visible());
    BOOST_TEST(!menu.navigate("menuNext"));
    BOOST_TEST(suspended.empty());
}

/**
 * Activating a key input captures rather than dispatching, and the key it is then given is
 * the whole answer.
 *
 * This is what pong's four Options items could not do: activating one used to fall through
 * a branch that did nothing, so the screen was there and inert.
 **/
BOOST_AUTO_TEST_CASE(a_key_input_item_captures_a_key_and_sends_it) {
    boost::shared_ptr<v3d::ui::Engine> ui = load(bindings);
    v3d::ui::GameMenu menu(ui, v3d::ui::GameMenu::Suspend());

    menu.toggle();
    BOOST_TEST(active(ui) == "Player 1 Up: ");
    BOOST_TEST(!menu.capturing());

    // activating it opens the capture rather than sending the command
    BOOST_TEST(menu.navigate("selectMenu"));
    BOOST_TEST(menu.capturing());

    // and while it is open the menu does not move under it
    BOOST_TEST(menu.navigate("menuNext"));
    BOOST_TEST(active(ui) == "Player 1 Up: ");

    // one key is the whole of a binding, so giving it closes the capture
    BOOST_TEST(menu.capture(std::string("w")));
    BOOST_TEST(!menu.capturing());

    // the item carries what it captured, which is what its event goes out with
    boost::shared_ptr<v3d::ui::component::Menu> component =
        boost::dynamic_pointer_cast<v3d::ui::component::Menu>(ui->container("game-menu")->get("main-menu"));
    boost::optional<v3d::event::EventData> value = (*component)[0]->value();
    BOOST_REQUIRE(value);
    BOOST_TEST(std::get<std::string>(value.get()) == "w");

    // and the label shows it, which is what makes a rebinding screen readable
    BOOST_TEST((*component)[0]->text() == "Player 1 Up: w");
}

/**
 * Nothing takes a value when no capture is open, so a key pressed over an action item is
 * not quietly recorded onto it.
 **/
BOOST_AUTO_TEST_CASE(a_capture_takes_nothing_until_it_is_open) {
    boost::shared_ptr<v3d::ui::Engine> ui = load(bindings);
    v3d::ui::GameMenu menu(ui, v3d::ui::GameMenu::Suspend());

    // not even while the menu is down
    BOOST_TEST(!menu.capture(std::string("w")));

    menu.toggle();
    BOOST_TEST(!menu.capture(std::string("w")));

    boost::shared_ptr<v3d::ui::component::Menu> component =
        boost::dynamic_pointer_cast<v3d::ui::component::Menu>(ui->container("game-menu")->get("main-menu"));
    BOOST_TEST(!(*component)[0]->value());
}

/**
 * Backing out of a capture abandons it, and leaves the menu where it was rather than
 * leaving the level the item sits on.
 **/
BOOST_AUTO_TEST_CASE(a_toggle_during_a_capture_abandons_it) {
    boost::shared_ptr<v3d::ui::Engine> ui = load(bindings);
    v3d::ui::GameMenu menu(ui, v3d::ui::GameMenu::Suspend());

    menu.toggle();
    BOOST_TEST(menu.navigate("selectMenu"));
    BOOST_REQUIRE(menu.capturing());

    menu.toggle();
    BOOST_TEST(!menu.capturing());
    // the menu is still up, and nothing was captured
    BOOST_TEST(menu.visible());

    boost::shared_ptr<v3d::ui::component::Menu> component =
        boost::dynamic_pointer_cast<v3d::ui::component::Menu>(ui->container("game-menu")->get("main-menu"));
    BOOST_TEST(!(*component)[0]->value());

    // and navigation moves again
    BOOST_TEST(menu.navigate("menuNext"));
    BOOST_TEST(active(ui) == "Resume");
}

BOOST_AUTO_TEST_SUITE_END()

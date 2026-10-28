/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <demos/pong/src/PongEngine.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string/join.hpp>
#include <boost/json.hpp>
#include <boost/test/unit_test.hpp>

namespace {

/**
 * Every key pong ships bound sends a command pong registers. The engine is built and never
 * started, so this needs no window.
 **/
BOOST_AUTO_TEST_CASE(every_binding_sends_a_handled_command) {
    const std::ifstream file("data/mappings.json");
    BOOST_REQUIRE(file.is_open());
    std::stringstream text;
    text << file.rdbuf();

    const PongEngine engine("");
    const std::vector<std::string> unhandled = engine.unhandled(boost::json::parse(text.str()).as_object());
    BOOST_TEST(unhandled.empty(), "nothing handles " << boost::algorithm::join(unhandled, ", "));
}

/**
 * Every "context::command" a menu document's items send, wherever in the document they are.
 **/
void menuCommands(const boost::json::value & value, std::vector<std::string>* commands) {
    if (value.is_array()) {
        for (const boost::json::value & element : value.as_array()) {
            menuCommands(element, commands);
        }
        return;
    }
    if (!value.is_object()) {
        return;
    }
    const boost::json::object & object = value.as_object();
    if (object.contains("command") && object.contains("context")) {
        commands->push_back(std::string(object.at("context").as_string()) + "::" + std::string(object.at("command").as_string()));
    }
    for (const auto & member : object) {
        menuCommands(member.value(), commands);
    }
}

/**
 * Every item pong's menu offers sends a command pong registers, and each game mode it offers is
 * a different game: Singleplayer and Co-op.
 **/
BOOST_AUTO_TEST_CASE(every_menu_item_sends_a_handled_command) {
    const std::ifstream file("data/vgui.json");
    BOOST_REQUIRE(file.is_open());
    std::stringstream text;
    text << file.rdbuf();
    std::vector<std::string> commands;
    menuCommands(boost::json::parse(text.str()), &commands);
    BOOST_REQUIRE(!commands.empty());

    const PongEngine engine("");
    for (const std::string & command : commands) {
        BOOST_TEST(engine.commands().has(command), "nothing handles " << command);
    }
    BOOST_TEST((std::find(commands.begin(), commands.end(), "ui::setMultiplayerMode") == commands.end()));
    BOOST_TEST(!engine.commands().has("ui::setMultiplayerMode"));
}

};  // namespace

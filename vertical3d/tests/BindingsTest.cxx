/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vertical3d/src/Controller.h>

#include <fstream>
#include <initializer_list>
#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string/join.hpp>
#include <boost/json.hpp>
#include <boost/test/unit_test.hpp>

namespace {

/**
 * Every key the editor ships bound sends a command the editor registers. The engine is built and
 * never started, so this needs no window.
 **/
BOOST_AUTO_TEST_CASE(every_binding_sends_a_handled_command) {
    const std::ifstream file("data/mappings.json");
    BOOST_REQUIRE(file.is_open());
    std::stringstream text;
    text << file.rdbuf();

    const v3d::editor::Controller engine("");
    const std::vector<std::string> unhandled = engine.unhandled(boost::json::parse(text.str()).as_object());
    BOOST_TEST(unhandled.empty(), "nothing handles " << boost::algorithm::join(unhandled, ", "));
}

/**
 * Collect the command every component under a ui entry sends, as "context::command".
 **/
void sent(const boost::json::value& entry, std::vector<std::string>* commands) {
    if (entry.is_array()) {
        for (const boost::json::value& item : entry.as_array()) {
            sent(item, commands);
        }
        return;
    }
    if (!entry.is_object()) {
        return;
    }
    const boost::json::object& object = entry.as_object();
    if (object.contains("context") && object.contains("command")) {
        commands->push_back(std::string(object.at("context").as_string()) + "::" + std::string(object.at("command").as_string()));
    }
    for (const char* children : { "components", "children" }) {
        if (object.contains(children)) {
            sent(object.at(children), commands);
        }
    }
}

/**
 * The dialogs the editor puts up, the file chooser and the prompt, send only commands the
 * editor registers. A button that sent anything else would leave its dialog up for good.
 **/
BOOST_AUTO_TEST_CASE(every_dialog_button_sends_a_handled_command) {
    const std::ifstream file("data/vgui.json");
    BOOST_REQUIRE(file.is_open());
    std::stringstream text;
    text << file.rdbuf();

    const boost::json::value document = boost::json::parse(text.str());
    std::vector<std::string> commands;
    for (const boost::json::value& container : document.at("containers").as_array()) {
        const std::string name(container.at("name").as_string());
        if (name == "chooser" || name == "prompt") {
            sent(container, &commands);
        }
    }
    BOOST_TEST(commands.size() >= 6U);

    const v3d::editor::Controller engine("");
    for (const std::string& command : commands) {
        BOOST_TEST(engine.commands().has(command), "nothing handles " << command);
    }
}

};  // namespace

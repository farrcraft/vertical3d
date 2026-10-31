/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/CameraProfiles.h>
#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/Controller.h>
#include <vertical3d/src/control/AreaChrome.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/ViewPort.h>

#include <fstream>
#include <initializer_list>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string/join.hpp>
#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec4.hpp>

namespace {

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
    for (const char* children : { "components", "children", "menus", "items", "buttons" }) {
        if (object.contains(children)) {
            sent(object.at(children), commands);
        }
    }
}

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
 * The dialogs the editor puts up, the file chooser, the prompt and the command search, send
 * only commands the editor registers. A button that sent anything else would leave its dialog
 * up for good.
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
        if (name == "chooser" || name == "prompt" || name == "search") {
            sent(container, &commands);
        }
    }
    BOOST_TEST(commands.size() >= 6U);

    const v3d::editor::Controller engine("");
    for (const std::string& command : commands) {
        BOOST_TEST(engine.commands().has(command), "nothing handles " << command);
    }
}

/**
 * Each fixed view the editor offers names a camera profile the editor ships, and the menu's camera
 * items are those views and the projection switch, each handled.
 **/
BOOST_AUTO_TEST_CASE(every_fixed_view_names_a_shipped_profile) {
    const std::ifstream cameras("data/cameras.json");
    BOOST_REQUIRE(cameras.is_open());
    std::stringstream text;
    text << cameras.rdbuf();
    v3d::engine::CameraProfiles profiles(boost::make_shared<v3d::log::Logger>());
    BOOST_REQUIRE(profiles.load(boost::json::parse(text.str()).as_object()));

    const v3d::editor::Controller engine("");
    std::set<std::string> offered{"view::camera::orthographic"};
    for (const v3d::editor::AreaChrome::FixedView& view : v3d::editor::AreaChrome::fixedViews()) {
        BOOST_TEST(profiles.has(view.profile), "no camera profile is called " << view.profile);
        offered.insert(view.command);
    }
    for (const std::string& command : offered) {
        BOOST_TEST(engine.commands().has(command), "nothing handles " << command);
    }

    const std::ifstream ui("data/vgui.json");
    BOOST_REQUIRE(ui.is_open());
    std::stringstream layout;
    layout << ui.rdbuf();
    std::vector<std::string> commands;
    sent(boost::json::parse(layout.str()).at("containers"), &commands);
    std::set<std::string> onMenu;
    for (const std::string& command : commands) {
        if (command.starts_with("view::camera::")) {
            onMenu.insert(command);
        }
    }
    BOOST_TEST((onMenu == offered));
}

/**
 * Every command a viewport's header sends, from its area menu and its view menus, is one the
 * editor registers.
 **/
BOOST_AUTO_TEST_CASE(every_header_command_is_handled) {
    const boost::shared_ptr<v3d::editor::AreaLayout> layout = boost::make_shared<v3d::editor::AreaLayout>(boost::make_shared<v3d::log::Logger>());
    BOOST_REQUIRE(layout->load(boost::json::parse(R"({"root": {"editors": [{"kind": "viewport", "camera": "Perspective"}]}})").as_object()));
    layout->resize(glm::vec4(0.0F, 0.0F, 400.0F, 300.0F));
    v3d::editor::AreaViews views(layout, [](const v3d::editor::AreaLayout::Editor&) {
        return boost::make_shared<v3d::editor::ViewPort>("Perspective", v3d::type::camera::Profile("Perspective"));
    });
    BOOST_REQUIRE(views.sync());

    std::vector<std::string> commands;
    sent(v3d::editor::AreaChrome::document(views, v3d::editor::SelectMask::Object).at("containers"), &commands);
    BOOST_TEST(commands.size() >= 30U);
    const v3d::editor::Controller engine("");
    for (const std::string& command : commands) {
        BOOST_TEST(engine.commands().has(command), "nothing handles " << command);
    }
}

};  // namespace

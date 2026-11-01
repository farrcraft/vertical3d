/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Context.h>
#include <api/event/Event.h>
#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/Length.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Label.h>
#include <api/ui/component/Bar.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/component/menu/Bar.h>
#include <api/ui/component/menu/Item.h>
#include <api/ui/component/menu/Menu.h>
#include <api/ui/paint/ComponentRenderer.h>

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

#include <boost/json/parse.hpp>
#include <boost/optional.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/make_shared.hpp>

#include "Documents.h"

namespace v3d::ui::documents {
namespace {

BOOST_AUTO_TEST_SUITE(theme_test)

/**
 * A button, a label and an icon are components a container can hold, and each carries the
 * box it asks for. position() and size() stay empty until something draws them.
 **/
BOOST_AUTO_TEST_CASE(a_container_holds_buttons_labels_and_icons) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "hud", "visible": true, "components": [
            { "type": "button", "name": "go", "label": "Go", "position": [10, 20], "size": [100, 30],
              "style": "flat", "context": "ui", "command": "quit", "toggle": true },
            { "type": "label", "name": "score", "label": "Score", "position": [5, 5] },
            { "type": "icon", "name": "logo", "source": "skins/logo.tga", "visible": false }
        ] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);

    const boost::shared_ptr<v3d::ui::Container> container = ui->container("hud");
    BOOST_REQUIRE(container);
    BOOST_CHECK_EQUAL(container->components().size(), 3U);

    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::dynamic_pointer_cast<v3d::ui::component::Button>(container->get("go"));
    BOOST_REQUIRE(button);
    BOOST_CHECK_EQUAL(button->label(), "Go");
    BOOST_CHECK(button->toggle());
    BOOST_CHECK_EQUAL(button->style(), "flat");
    BOOST_CHECK(button->layout().x.unit() == v3d::ui::Length::Unit::Pixels);
    BOOST_CHECK_CLOSE(button->layout().x.value(), 10.0F, 0.001F);
    BOOST_CHECK_CLOSE(button->layout().height.value(), 30.0F, 0.001F);
    BOOST_CHECK_CLOSE(button->position().x, 0.0F, 0.001F);
    BOOST_CHECK_EQUAL(button->event().str(), "ui::quit");

    const boost::shared_ptr<v3d::ui::component::Label> label =
        boost::dynamic_pointer_cast<v3d::ui::component::Label>(container->get("score"));
    BOOST_REQUIRE(label);
    BOOST_CHECK_EQUAL(label->text(), "Score");

    // visible is read for a component the same way it is for a container
    BOOST_REQUIRE(container->get("logo"));
    BOOST_CHECK(!container->get("logo")->visible());
}

/**
 * Any component may carry a tooltip. A menu item may name an icon, and a separator needs no
 * label.
 **/
BOOST_AUTO_TEST_CASE(tooltips_separators_and_item_icons_load) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "hud", "visible": true, "components": [
            { "type": "button", "name": "go", "label": "Go", "tooltip": "Start the game" },
            { "type": "menu", "name": "main", "items": [
                { "label": "Open", "type": "action", "icon": "icons/open.png" },
                { "type": "separator" },
                { "label": "Quit", "type": "action" }
            ] }
        ] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);
    BOOST_CHECK_EQUAL(ui->container("hud")->get("go")->tooltip(), "Start the game");

    const boost::shared_ptr<v3d::ui::component::menu::Menu> menu =
        boost::dynamic_pointer_cast<v3d::ui::component::menu::Menu>(ui->container("hud")->get("main"));
    BOOST_REQUIRE(menu);
    BOOST_REQUIRE_EQUAL(menu->count(), 3U);
    BOOST_CHECK_EQUAL((*menu)[0]->icon(), "icons/open.png");
    BOOST_CHECK((*menu)[1]->separator());
    BOOST_CHECK((*menu)[1]->label().empty());
}

/**
 * A menu bar reads its opacity, and each of its menus an icon, a drop mark and whether its title
 * is set apart. A menu that names none of them has none.
 **/
BOOST_AUTO_TEST_CASE(a_menu_bar_reads_its_opacity_and_its_titles) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "hud", "visible": true, "components": [
            { "type": "menubar", "name": "header", "docked": false, "opacity": 0.8, "menus": [
                { "label": "Viewport", "icon": "icon:editor-viewport", "drop": true, "items": [] },
                { "label": "Front", "emphasis": true, "accent": true, "items": [] },
                { "label": "Flat", "items": [] }
            ] }
        ] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);
    const boost::shared_ptr<v3d::ui::component::menu::Bar> bar =
        boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(ui->container("hud")->get("header"));
    BOOST_REQUIRE(bar);
    BOOST_CHECK_CLOSE(bar->opacity(), 0.8F, 0.001F);
    BOOST_REQUIRE_EQUAL(bar->count(), 3U);
    BOOST_CHECK_EQUAL(bar->menu(0)->icon(), "icon:editor-viewport");
    BOOST_CHECK(bar->menu(0)->drop());
    BOOST_CHECK(!bar->menu(0)->emphasis());
    BOOST_CHECK(bar->menu(1)->emphasis());
    BOOST_CHECK(bar->menu(1)->accent());
    BOOST_CHECK(!bar->menu(0)->accent());
    BOOST_CHECK(bar->menu(2)->icon().empty());
    BOOST_CHECK(!bar->menu(2)->drop());
}

/**
 * A container dims what is behind it only when its document asks it to.
 **/
BOOST_AUTO_TEST_CASE(a_container_may_ask_for_a_backdrop) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [
            { "name": "hud", "visible": true, "components": [] },
            { "name": "pause", "visible": false, "backdrop": true, "components": [] }
        ]
    })", &loaded);
    BOOST_REQUIRE(loaded);
    BOOST_REQUIRE(ui->container("hud"));
    BOOST_REQUIRE(ui->container("pause"));
    BOOST_CHECK(!ui->container("hud")->backdrop());
    BOOST_CHECK(ui->container("pause")->backdrop());
}

/**
 * A command's data rides on the event it sends, from a button or a menu item. Data of another
 * kind is ignored, and the command is kept.
 **/
BOOST_AUTO_TEST_CASE(a_command_may_carry_data) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "hud", "visible": true, "components": [
            { "type": "button", "name": "seven", "label": "7", "context": "ui", "command": "pick", "data": 7 },
            { "type": "button", "name": "named", "label": "N", "context": "ui", "command": "pick", "data": "north" },
            { "type": "button", "name": "odd", "label": "O", "context": "ui", "command": "pick", "data": [1] },
            { "type": "menubar", "name": "bar", "menus": [ { "label": "Area", "items": [
                { "label": "Split", "type": "action", "context": "ui", "command": "split", "data": true } ] } ] }
        ] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);
    const boost::shared_ptr<v3d::ui::Container> hud = ui->container("hud");
    const auto data = [&hud](const char* name) {
        return boost::dynamic_pointer_cast<v3d::ui::component::Button>(hud->get(name))->event().data();
    };
    BOOST_REQUIRE(data("seven"));
    BOOST_CHECK(std::get<int>(*data("seven")) == 7);
    BOOST_REQUIRE(data("named"));
    BOOST_CHECK(std::get<std::string>(*data("named")) == "north");
    BOOST_CHECK(!data("odd"));
    BOOST_CHECK_EQUAL(boost::dynamic_pointer_cast<v3d::ui::component::Button>(hud->get("odd"))->event().str(), "ui::pick");

    const boost::shared_ptr<v3d::ui::component::menu::Bar> bar =
        boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(hud->get("bar"));
    BOOST_REQUIRE(bar);
    const boost::optional<v3d::event::EventData> split = (*bar->menu(0))[0]->event().data();
    BOOST_REQUIRE(split);
    BOOST_CHECK(std::get<bool>(*split));
}

/**
 * A menu may name its title. A submenu is titled by the item that opens it unless it names a
 * title of its own, and a menu naming none has none.
 **/
BOOST_AUTO_TEST_CASE(a_menu_names_its_title_and_a_submenu_takes_its_items) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "game-menu", "visible": false, "components": [
            { "type": "menu", "name": "main-menu", "title": "Pong", "items": [
                { "label": "Options", "type": "submenu", "items": [
                    { "label": "Rounds: ", "type": "numeric_input", "command": "rounds", "context": "ui" } ] },
                { "label": "Keys", "type": "submenu", "title": "Controls", "items": [
                    { "label": "Up: ", "type": "key_input", "command": "up", "context": "ui" } ] }
            ] },
            { "type": "menu", "name": "plain", "items": [
                { "label": "Quit", "type": "action", "command": "quit", "context": "ui" } ] }
        ] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);
    const boost::shared_ptr<v3d::ui::Container> holder = ui->container("game-menu");
    const boost::shared_ptr<v3d::ui::component::menu::Menu> menu =
        boost::dynamic_pointer_cast<v3d::ui::component::menu::Menu>(holder->get("main-menu"));
    BOOST_REQUIRE(menu);
    BOOST_CHECK_EQUAL(menu->title(), "Pong");
    BOOST_REQUIRE((*menu)[0]->submenu() && (*menu)[1]->submenu());
    BOOST_CHECK_EQUAL((*menu)[0]->submenu()->title(), "Options");
    BOOST_CHECK_EQUAL((*menu)[1]->submenu()->title(), "Controls");
    const boost::shared_ptr<v3d::ui::component::menu::Menu> plain =
        boost::dynamic_pointer_cast<v3d::ui::component::menu::Menu>(holder->get("plain"));
    BOOST_REQUIRE(plain);
    BOOST_CHECK(plain->title().empty());
}

/**
 * A slider reads its range and its starting value, clamped to the range, and an item reads
 * the command it binds as a context and a command. The context is the part before the first
 * "::", so a command may hold "::" itself. A binding that names no context is ignored.
 **/
BOOST_AUTO_TEST_CASE(a_settings_item_reads_its_range_and_what_it_binds) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "game-menu", "visible": false, "components": [
            { "type": "menu", "name": "main-menu", "items": [
                { "label": "Rounds", "type": "slider", "command": "rounds", "context": "ui",
                  "minimum": 1, "maximum": 9, "step": 2, "value": 12 },
                { "label": "Up", "type": "key_input", "command": "setUp", "context": "ui", "binds": "pong::up" },
                { "label": "Down", "type": "key_input", "command": "setDown", "context": "ui", "binds": "down" },
                { "label": "Front", "type": "key_input", "command": "setFront", "context": "ui", "binds": "editor::camera::front" }
            ] }
        ] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);
    const boost::shared_ptr<v3d::ui::component::menu::Menu> menu =
        boost::dynamic_pointer_cast<v3d::ui::component::menu::Menu>(ui->container("game-menu")->get("main-menu"));
    BOOST_REQUIRE(menu);
    const v3d::ui::component::menu::Item::Range& span = (*menu)[0]->range();
    BOOST_CHECK_EQUAL(span.minimum, 1);
    BOOST_CHECK_EQUAL(span.maximum, 9);
    BOOST_CHECK_EQUAL(span.step, 2);
    BOOST_CHECK_EQUAL((*menu)[0]->shown(), "9");

    const v3d::event::Event& binds = (*menu)[1]->binds();
    BOOST_REQUIRE(binds.context());
    BOOST_CHECK_EQUAL(binds.context()->name(), "pong");
    BOOST_CHECK_EQUAL(binds.name(), "up");
    BOOST_CHECK(!(*menu)[2]->binds().context());
    const v3d::event::Event& nested = (*menu)[3]->binds();
    BOOST_REQUIRE(nested.context());
    BOOST_CHECK_EQUAL(nested.context()->name(), "editor");
    BOOST_CHECK_EQUAL(nested.name(), "camera::front");
}

/**
 * A menu bar is docked unless its document says otherwise, and only one that is not docked
 * reads a layout.
 **/
BOOST_AUTO_TEST_CASE(a_menu_bar_may_be_undocked) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "hud", "visible": true, "components": [
            { "type": "menubar", "name": "top", "position": [5, 6], "menus": [] },
            { "type": "menubar", "name": "header", "docked": false, "position": [5, 6], "size": [200, 28], "menus": [] }
        ] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);
    const boost::shared_ptr<v3d::ui::Container> hud = ui->container("hud");
    const boost::shared_ptr<v3d::ui::component::menu::Bar> top =
        boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(hud->get("top"));
    const boost::shared_ptr<v3d::ui::component::menu::Bar> header =
        boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(hud->get("header"));
    BOOST_REQUIRE(top && header);
    BOOST_CHECK(top->docked());
    BOOST_CHECK(top->layout().x.unit() == v3d::ui::Length::Unit::Auto);
    BOOST_CHECK(!header->docked());
    BOOST_CHECK_CLOSE(header->layout().x.value(), 5.0F, 0.001F);
    BOOST_CHECK_CLOSE(header->layout().width.value(), 200.0F, 0.001F);

    // the docked bar takes a strip off the top, with the rule under it, and the other nothing
    v3d::ui::paint::ComponentRenderer drawing = renderer();
    BOOST_CHECK_CLOSE(drawing.insets(*ui).y, drawing.dressing().barHeight + 1.0F, 0.001F);
}

/**
 * Containers can be added to a loaded ui and taken out again by name. A document naming a
 * container already held adds nothing.
 **/
BOOST_AUTO_TEST_CASE(containers_are_added_and_removed) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "hud", "visible": true, "components": [] } ]
    })", &loaded);
    BOOST_REQUIRE(loaded);

    BOOST_REQUIRE(ui->add(boost::json::parse(R"({ "containers": [
        { "name": "area-1", "visible": true, "components": [
            { "type": "button", "name": "go", "label": "Go", "context": "ui", "command": "go" } ] },
        { "name": "area-2", "visible": true, "components": [] } ] })").as_object()));
    BOOST_REQUIRE_EQUAL(ui->containers().size(), 3U);
    BOOST_CHECK_EQUAL(ui->containers()[1]->name(), "area-1");
    BOOST_CHECK_EQUAL(ui->containers()[2]->name(), "area-2");

    BOOST_CHECK(!ui->add(boost::json::parse(R"({ "containers": [
        { "name": "area-3", "visible": true, "components": [] },
        { "name": "hud", "visible": true, "components": [] } ] })").as_object()));
    BOOST_CHECK(!ui->add(boost::json::parse(R"({ "containers": [ { "name": "area-4" } ] })").as_object()));
    BOOST_CHECK_EQUAL(ui->containers().size(), 3U);

    // what is added before a container is held, and drawn, before it
    BOOST_REQUIRE(ui->add(boost::json::parse(R"({ "containers": [ { "name": "under", "visible": true, "components": [] } ] })").as_object(), "hud"));
    BOOST_CHECK_EQUAL(ui->containers()[0]->name(), "under");
    BOOST_CHECK_EQUAL(ui->containers()[1]->name(), "hud");
    BOOST_REQUIRE(ui->remove("under"));

    // a component of a container taken out stops holding the cursor's rest
    const boost::shared_ptr<v3d::ui::Component> go = ui->container("area-1")->get("go");
    ui->hovered(go);
    BOOST_REQUIRE(ui->remove("area-1"));
    BOOST_CHECK(!ui->container("area-1"));
    BOOST_CHECK(!ui->hovered());
    BOOST_CHECK(!ui->remove("area-1"));
    BOOST_CHECK_EQUAL(ui->containers().size(), 2U);
}

/**
 * A component type the library does not recognise is refused rather than skipped, so a typo in a
 * config is not a component silently missing from the window.
 **/
BOOST_AUTO_TEST_CASE(an_unknown_component_type_is_refused) {
    bool loaded = true;
    load(R"({
        "themes": [ { "name": "dark" } ],
        "containers": [ { "name": "hud", "visible": true, "components": [
            { "type": "carousel", "name": "spinner" }
        ] } ]
    })", &loaded);
    BOOST_CHECK(!loaded);
}

/**
 * A member the document cannot do without, missing or holding the wrong type, refuses the
 * document rather than throwing out of the load.
 **/
BOOST_AUTO_TEST_CASE(a_required_member_of_the_wrong_type_is_refused) {
    const char* const documents[] = {
        R"({ "containers": [] })",
        R"({ "themes": [] })",
        R"({ "themes": 5, "containers": [] })",
        R"({ "themes": [ { "name": 5 } ], "containers": [] })",
        R"({ "themes": [ { "name": "a", "styles": [ { "class": 5, "name": "s" } ] } ], "containers": [] })",
        R"({ "themes": [ { "name": "a", "styles": [ { "class": "c", "name": "s",
            "numbers": [ { "name": 5, "value": 1 } ] } ] } ], "containers": [] })",
        R"({ "themes": [], "containers": [ { "name": 5, "visible": true, "components": [] } ] })",
        R"({ "themes": [], "containers": [ { "name": "c", "visible": "yes", "components": [] } ] })",
        R"({ "themes": [], "containers": [ { "name": "c", "visible": true } ] })",
        R"({ "themes": [], "containers": [ { "name": "c", "visible": true, "components": [
            { "type": 5, "name": "b" } ] } ] })",
        R"({ "themes": [], "containers": [ { "name": "c", "visible": true, "components": [
            { "type": "button", "name": [] } ] } ] })",
        R"({ "themes": [], "containers": [ { "name": "c", "visible": true, "components": [
            { "type": "menu", "name": "m", "items": [ { "label": 5, "type": "action" } ] } ] } ] })",
        R"({ "themes": [], "containers": [ { "name": "c", "visible": true, "components": [
            { "type": "menubar", "name": "m" } ] } ] })",
        R"({ "themes": [], "containers": [ { "name": "c", "visible": true, "components": [
            { "type": "list", "name": "l", "items": [ "one", 2 ] } ] } ] })"
    };
    for (const char* const document : documents) {
        BOOST_TEST_CONTEXT(document) {
            bool loaded = true;
            BOOST_CHECK_NO_THROW(load(document, &loaded));
            BOOST_CHECK(!loaded);
        }
    }
}

/**
 * An optional member of the wrong type is logged and ignored, so the document loads and the
 * setting keeps its default.
 **/
BOOST_AUTO_TEST_CASE(an_optional_member_of_the_wrong_type_is_ignored) {
    bool loaded = false;
    boost::shared_ptr<v3d::ui::Interface> ui;
    BOOST_CHECK_NO_THROW(ui = load(R"({
        "themes": [ { "name": "dark", "styles": [ { "class": "button", "name": "s", "state": 5 } ] } ],
        "containers": [ { "name": "hud", "visible": true, "components": [
            { "type": "button", "name": "go", "label": 5, "icon": [], "style": 3, "depth": -1,
              "anchor": 2, "command": 5, "context": "ui" },
            { "type": "bar", "name": "health", "fraction": "full", "direction": 7 },
            { "type": "textbox", "name": "entry", "text": 4, "limit": -3 },
            { "type": "list", "name": "rows", "selected": "first" }
        ] } ]
    })", &loaded));
    BOOST_REQUIRE(loaded);

    const boost::shared_ptr<v3d::ui::component::Button> button =
        boost::dynamic_pointer_cast<v3d::ui::component::Button>(ui->container("hud")->get("go"));
    BOOST_REQUIRE(button);
    BOOST_CHECK(button->label().empty());
    BOOST_CHECK(button->style().empty());
    BOOST_CHECK(!button->event().context());
}

/**
 * A required member that names a word it does not take refuses the document, as one of the
 * wrong type does.
 **/
BOOST_AUTO_TEST_CASE(a_required_member_outside_its_words_is_refused) {
    bool loaded = true;
    BOOST_CHECK_NO_THROW(load(R"({ "themes": [], "containers": [ { "name": "c", "visible": true, "components": [
        { "type": "menu", "name": "m", "items": [ { "label": "Go", "type": "teleport" } ] } ] } ] })", &loaded));
    BOOST_CHECK(!loaded);
}

/**
 * An optional member that names a word it does not take is logged and ignored, as one of the
 * wrong type is, so the document loads and the setting keeps its default.
 **/
BOOST_AUTO_TEST_CASE(an_optional_member_outside_its_words_keeps_its_default) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({ "themes": [], "containers": [
        { "name": "hud", "visible": true, "components": [
            { "type": "toolbar", "name": "tools", "edge": "bottom", "buttons": [] },
            { "type": "bar", "name": "health", "direction": "sideways", "anchor": "middle" }
        ] } ] })", &loaded);
    BOOST_REQUIRE(loaded);

    const boost::shared_ptr<v3d::ui::component::Toolbar> toolbar =
        boost::dynamic_pointer_cast<v3d::ui::component::Toolbar>(ui->container("hud")->get("tools"));
    BOOST_REQUIRE(toolbar);
    BOOST_CHECK((toolbar->edge() == v3d::ui::component::Toolbar::Edge::Top));

    const boost::shared_ptr<v3d::ui::component::Bar> bar =
        boost::dynamic_pointer_cast<v3d::ui::component::Bar>(ui->container("hud")->get("health"));
    BOOST_REQUIRE(bar);
    BOOST_CHECK((bar->direction() == v3d::ui::component::Bar::Direction::Horizontal));
    BOOST_CHECK((bar->layout().anchor == v3d::ui::Layout::Anchor::TopLeft));
}

/**
 * An optional array of the wrong type is ignored like any other optional member, so a
 * component whose children are not an array loads with none.
 **/
BOOST_AUTO_TEST_CASE(an_optional_array_of_the_wrong_type_is_ignored) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({ "themes": [], "containers": [
        { "name": "hud", "visible": true, "components": [
            { "type": "panel", "name": "plate", "children": 5, "position": [ "left", 4 ] }
        ] } ] })", &loaded);
    BOOST_REQUIRE(loaded);

    const boost::shared_ptr<v3d::ui::Component> panel = ui->container("hud")->get("plate");
    BOOST_REQUIRE(panel);
    BOOST_CHECK(panel->children().empty());
    // the length that reads is kept, and the one that does not is left as it was
    BOOST_CHECK((panel->layout().x.unit() == v3d::ui::Length::Unit::Auto));
    BOOST_CHECK_CLOSE(panel->layout().y.value(), 4.0F, 0.001F);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
};  // namespace v3d::ui::documents

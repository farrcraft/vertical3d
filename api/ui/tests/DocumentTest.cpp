/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/Handle.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/type/geometry/Bound2D.h>
#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/Image.h>
#include <api/ui/Length.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Icon.h>
#include <api/ui/component/Label.h>
#include <api/ui/component/Bar.h>
#include <api/ui/component/Panel.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/paint/ComponentRenderer.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/style/Button.h>
#include <api/ui/style/Style.h>
#include <api/ui/style/Theme.h>
#include <api/ui/style/property/Color.h>
#include <api/ui/style/property/Image.h>
#include <api/ui/style/property/Number.h>

#include <cstddef>
#include <string>
#include <vector>

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
 * A component type the library does not know is refused rather than skipped, so a typo in a
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
            { "type": "list", "name": "l", "items": [ "one", 2 ] } ] } ] })",
        R"({ "themes": [], "containers": [ { "name": "c", "visible": true, "components": [
            { "type": "icon", "name": "i", "source": 5 } ] } ] })"
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

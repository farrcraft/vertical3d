/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ui/Interface.h>
#include <api/ui/Image.h>
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
 * A theme carries styles, a style carries properties of three kinds, and each is read as the
 * kind of the array it was written in. Nothing draws with a font, so a style's fonts are
 * not read.
 **/
BOOST_AUTO_TEST_CASE(a_theme_loads_its_styles_and_their_properties) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(themedDocument, &loaded);
    BOOST_REQUIRE(loaded);

    const boost::shared_ptr<v3d::ui::style::Theme> dark = ui->theme("dark");
    BOOST_REQUIRE(dark);
    BOOST_CHECK_EQUAL(dark->getStyleSet("", "").size(), 3U);

    const std::vector<boost::shared_ptr<v3d::ui::style::Style>> chrome = dark->getStyleSet("", "ui");
    BOOST_REQUIRE_EQUAL(chrome.size(), 1U);

    const boost::shared_ptr<v3d::ui::style::property::Color> panel =
        boost::dynamic_pointer_cast<v3d::ui::style::property::Color>(chrome.front()->property("panel", "color"));
    BOOST_REQUIRE(panel);
    BOOST_CHECK_CLOSE(panel->value().a, 0.5F, 0.001F);

    const boost::shared_ptr<v3d::ui::style::property::Number> height =
        boost::dynamic_pointer_cast<v3d::ui::style::property::Number>(chrome.front()->property("bar-height", "number"));
    BOOST_REQUIRE(height);
    BOOST_CHECK_CLOSE(height->value(), 40.0F, 0.001F);

    const std::vector<boost::shared_ptr<v3d::ui::style::Style>> labels = dark->getStyleSet("", "label");
    BOOST_REQUIRE_EQUAL(labels.size(), 1U);
    BOOST_CHECK(!labels.front()->property("label", "font"));
}

/**
 * A button style is told apart by its state as well as by its name, and an image property
 * carries its source.
 **/
BOOST_AUTO_TEST_CASE(a_button_style_carries_a_state_and_its_images) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(themedDocument, &loaded);
    BOOST_REQUIRE(loaded);

    const std::vector<boost::shared_ptr<v3d::ui::style::Style>> buttons = ui->theme("dark")->getStyleSet("", "button");
    BOOST_REQUIRE_EQUAL(buttons.size(), 1U);

    const boost::shared_ptr<v3d::ui::style::Button> styled =
        boost::dynamic_pointer_cast<v3d::ui::style::Button>(buttons.front());
    BOOST_REQUIRE(styled);
    BOOST_CHECK((styled->state() == v3d::ui::style::Button::State::Normal));

    const boost::shared_ptr<v3d::ui::style::property::Image> corner =
        boost::dynamic_pointer_cast<v3d::ui::style::property::Image>(styled->property("top-left", "image"));
    BOOST_REQUIRE(corner);
    BOOST_CHECK_EQUAL(corner->source(), "skins/tl.tga");
    // nothing has uploaded it, so the handle it will draw with is unset
    BOOST_CHECK(!corner->image().valid());
}

/**
 * The document names which theme is active. Without the field the first one loaded is.
 **/
BOOST_AUTO_TEST_CASE(the_document_names_the_active_theme) {
    bool loaded = false;
    boost::shared_ptr<v3d::ui::Interface> ui = load(themedDocument, &loaded);
    BOOST_REQUIRE(loaded);
    BOOST_REQUIRE(ui->activeTheme());
    BOOST_CHECK_EQUAL(ui->activeTheme()->name(), "light");

    ui = load(R"({ "themes": [ { "name": "first" }, { "name": "second" } ], "containers": [] })", &loaded);
    BOOST_REQUIRE(loaded);
    BOOST_REQUIRE(ui->activeTheme());
    BOOST_CHECK_EQUAL(ui->activeTheme()->name(), "first");
}

/**
 * A theme naming one that was never loaded is refused rather than leaving the ui drawn with
 * a theme the document did not ask for.
 **/
BOOST_AUTO_TEST_CASE(an_unknown_active_theme_is_refused) {
    bool loaded = true;
    load(R"({ "themes": [ { "name": "first" } ], "theme": "missing", "containers": [] })", &loaded);
    BOOST_CHECK(!loaded);
}

/**
 * Every colour and metric the theme does not name keeps the default it had, so a theme that
 * names one colour changes one thing.
 **/
BOOST_AUTO_TEST_CASE(a_theme_overrides_what_it_names_and_no_more) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(themedDocument, &loaded);
    BOOST_REQUIRE(loaded);

    v3d::ui::paint::ComponentRenderer drawing = renderer();
    const v3d::ui::paint::Dressing defaults;

    drawing.theme(ui->theme("dark"));

    BOOST_CHECK_CLOSE(drawing.dressing().panel.b, 0.3F, 0.001F);
    BOOST_CHECK_CLOSE(drawing.dressing().barHeight, 40.0F, 0.001F);
    // the style named neither, so both are what they were
    BOOST_CHECK_CLOSE(drawing.dressing().lineHeight, defaults.lineHeight, 0.001F);
    BOOST_CHECK_CLOSE(drawing.dressing().border.r, defaults.border.r, 0.001F);
}

/**
 * A theme with no styles in it draws exactly as no theme at all does.
 **/
BOOST_AUTO_TEST_CASE(a_nameless_theme_changes_nothing) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(
        R"({ "themes": [ { "name": "plain" } ], "containers": [] })", &loaded);
    BOOST_REQUIRE(loaded);

    v3d::ui::paint::ComponentRenderer drawing = renderer();
    const v3d::ui::paint::Dressing defaults;
    drawing.theme(ui->activeTheme());

    BOOST_CHECK_CLOSE(drawing.dressing().barHeight, defaults.barHeight, 0.001F);
    BOOST_CHECK_CLOSE(drawing.dressing().panel.a, defaults.panel.a, 0.001F);
}

/**
 * A theme's optional members of the wrong type are logged and ignored: the active theme's
 * name, a theme's styles, a style's array of properties, and a button style's state, which
 * reads as the normal one.
 **/
BOOST_AUTO_TEST_CASE(a_theme_member_of_the_wrong_type_is_ignored) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
        "themes": [
            { "name": "plain", "styles": 5 },
            { "name": "dark", "styles": [
                { "class": "ui", "name": "default", "colors": 5 },
                { "class": "button", "name": "default", "state": "glowing" } ] }
        ],
        "theme": 5,
        "containers": []
    })", &loaded);
    BOOST_REQUIRE(loaded);

    BOOST_REQUIRE(ui->theme("plain"));
    BOOST_CHECK(ui->theme("plain")->getStyleSet("", "").empty());
    // a "theme" that is not a name names none, so the first theme is active
    BOOST_CHECK(ui->activeTheme() == ui->theme("plain"));

    const std::vector<boost::shared_ptr<v3d::ui::style::Style>> buttons = ui->theme("dark")->getStyleSet("", "button");
    BOOST_REQUIRE_EQUAL(buttons.size(), 1U);
    const boost::shared_ptr<v3d::ui::style::Button> button = boost::dynamic_pointer_cast<v3d::ui::style::Button>(buttons.front());
    BOOST_REQUIRE(button);
    BOOST_CHECK((button->state() == v3d::ui::style::Button::State::Normal));
}

/**
 * A colour's value is required, so a value that is not four numbers refuses the document
 * rather than reading as some colour.
 **/
BOOST_AUTO_TEST_CASE(a_colour_that_is_not_four_numbers_is_refused) {
    const char* const documents[] = {
        R"({ "themes": [ { "name": "a", "styles": [ { "class": "ui", "name": "s",
            "colors": [ { "name": "panel", "value": [0.1, 0.2] } ] } ] } ], "containers": [] })",
        R"({ "themes": [ { "name": "a", "styles": [ { "class": "ui", "name": "s",
            "colors": [ { "name": "panel", "value": [0.1, 0.2, "blue", 1] } ] } ] } ], "containers": [] })",
        R"({ "themes": [ { "name": "a", "styles": [ { "class": "ui", "name": "s",
            "colors": [ { "name": "panel" } ] } ] } ], "containers": [] })"
    };
    for (const char* const document : documents) {
        BOOST_TEST_CONTEXT(document) {
            bool loaded = true;
            BOOST_CHECK_NO_THROW(load(document, &loaded));
            BOOST_CHECK(!loaded);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
};  // namespace v3d::ui::documents

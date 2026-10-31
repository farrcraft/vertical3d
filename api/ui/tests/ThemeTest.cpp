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

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <glm/vec4.hpp>

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

/**
 * A theme put beneath keeps every value the theme over it names and fills in the rest, for a
 * style of the same class and name, and of the same state for a button. A style with no match
 * is added after the theme's own, so the theme's own first style of a class still dresses a
 * component that names none.
 **/
BOOST_AUTO_TEST_CASE(a_theme_beneath_fills_in_what_the_one_over_it_does_not_name) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({
        "themes": [ { "name": "game", "styles": [
            { "class": "ui", "name": "default", "colors": [ { "name": "accent", "value": [1, 0, 0, 1] } ] },
            { "class": "button", "name": "go", "state": "hover", "numbers": [ { "name": "corner", "value": 3 } ] }
        ] } ],
        "containers": [] })", &loaded);
    BOOST_REQUIRE(loaded);
    BOOST_REQUIRE(ui->beneath(boost::json::parse(R"({
        "themes": [ { "name": "shell", "styles": [
            { "class": "panel", "name": "plate", "numbers": [ { "name": "radius", "value": 2 } ] },
            { "class": "ui", "name": "default",
              "colors": [ { "name": "accent", "value": [0, 0, 1, 1] } ],
              "numbers": [ { "name": "radius", "value": 8 } ] },
            { "class": "button", "name": "go", "state": "normal", "numbers": [ { "name": "corner", "value": 5 } ] }
        ] } ] })").as_object()));

    const boost::shared_ptr<v3d::ui::style::Theme> theme = ui->activeTheme();
    BOOST_REQUIRE(theme);
    BOOST_TEST(theme->name() == "game");
    const std::vector<boost::shared_ptr<v3d::ui::style::Style>> chrome = theme->getStyleSet("", "ui");
    BOOST_REQUIRE_EQUAL(chrome.size(), 1U);
    glm::vec4 accent(0.0F);
    v3d::ui::style::readColour(chrome.front(), "accent", &accent);
    BOOST_CHECK_CLOSE(accent.r, 1.0F, 0.001F);
    float radius = 0.0F;
    v3d::ui::style::readMetric(chrome.front(), "radius", &radius);
    BOOST_CHECK_CLOSE(radius, 8.0F, 0.001F);

    BOOST_CHECK_EQUAL(theme->getStyleSet("", "panel").size(), 1U);
    // the hover skin and the normal one are two styles, not one filled in from the other
    const std::vector<boost::shared_ptr<v3d::ui::style::Style>> buttons = theme->getStyleSet("go", "button");
    BOOST_REQUIRE_EQUAL(buttons.size(), 2U);
    float corner = 0.0F;
    v3d::ui::style::readMetric(buttons.front(), "corner", &corner);
    BOOST_CHECK_CLOSE(corner, 3.0F, 0.001F);
}

/**
 * Each theme a style is put beneath takes its own copy of the style, of the same kind. Filling
 * in one theme's copy later leaves the other themes and the theme beneath as they were.
 **/
BOOST_AUTO_TEST_CASE(a_style_put_beneath_two_themes_is_not_shared_between_them) {
    const auto plate = [](const std::string& name, float value) {
        const boost::shared_ptr<v3d::ui::style::Style> style =
            boost::make_shared<v3d::ui::style::Style>(v3d::ui::style::Selector{ .name = "plate", .className = "panel" });
        style->addProperty(boost::make_shared<v3d::ui::style::property::Number>(name, value), "number");
        return style;
    };
    v3d::ui::style::Theme lower("lower");
    lower.addStyle(plate("radius", 2.0F));
    lower.addStyle(boost::make_shared<v3d::ui::style::Button>("go", v3d::ui::style::Button::State::Hover));
    v3d::ui::style::Theme one("one");
    v3d::ui::style::Theme two("two");
    one.beneath(lower);
    two.beneath(lower);

    v3d::ui::style::Theme further("further");
    further.addStyle(plate("corner", 4.0F));
    one.beneath(further);

    float corner = 0.0F;
    v3d::ui::style::readMetric(one.getStyleSet("plate", "panel").front(), "corner", &corner);
    BOOST_CHECK_CLOSE(corner, 4.0F, 0.001F);
    corner = 0.0F;
    v3d::ui::style::readMetric(two.getStyleSet("plate", "panel").front(), "corner", &corner);
    BOOST_CHECK_CLOSE(corner, 0.0F, 0.001F);
    v3d::ui::style::readMetric(lower.getStyleSet("plate", "panel").front(), "corner", &corner);
    BOOST_CHECK_CLOSE(corner, 0.0F, 0.001F);
    float radius = 0.0F;
    v3d::ui::style::readMetric(two.getStyleSet("plate", "panel").front(), "radius", &radius);
    BOOST_CHECK_CLOSE(radius, 2.0F, 0.001F);

    const boost::shared_ptr<v3d::ui::style::Button> button =
        boost::dynamic_pointer_cast<v3d::ui::style::Button>(two.getStyleSet("go", "button").front());
    BOOST_REQUIRE(button);
    BOOST_CHECK(button->state() == v3d::ui::style::Button::State::Hover);
}

/**
 * A ui with no theme of its own takes the themes put beneath it, and the first is active. A
 * document that does not read puts nothing beneath.
 **/
BOOST_AUTO_TEST_CASE(a_ui_with_no_theme_takes_the_one_put_beneath) {
    bool loaded = false;
    const boost::shared_ptr<v3d::ui::Interface> ui = load(R"({ "themes": [], "containers": [] })", &loaded);
    BOOST_REQUIRE(loaded);
    BOOST_TEST(!ui->activeTheme());
    BOOST_TEST(!ui->beneath(boost::json::parse(R"({ "containers": [] })").as_object()));
    BOOST_TEST(!ui->activeTheme());

    BOOST_REQUIRE(ui->beneath(boost::json::parse(R"({ "themes": [ { "name": "shell" } ] })").as_object()));
    BOOST_REQUIRE(ui->activeTheme());
    BOOST_TEST(ui->activeTheme()->name() == "shell");
}

/**
 * A panel's opacity multiplies the panel's alpha, whether the panel was named or derived from
 * an accent, and switching away puts the opaque panel back.
 **/
BOOST_AUTO_TEST_CASE(a_panels_opacity_applies_over_its_colour) {
    v3d::ui::paint::ComponentRenderer drawn = renderer();
    const float opaque = drawn.dressing().panel.a;

    const boost::shared_ptr<v3d::ui::style::Theme> named = boost::make_shared<v3d::ui::style::Theme>("named");
    const boost::shared_ptr<v3d::ui::style::Style> plain = boost::make_shared<v3d::ui::style::Style>(
        v3d::ui::style::Selector{.name = "default", .className = "ui"});
    plain->addProperty(boost::make_shared<v3d::ui::style::property::Number>("panel-opacity", 0.9F), "number");
    named->addStyle(plain);
    drawn.theme(named);
    BOOST_CHECK_CLOSE(drawn.dressing().panel.a, opaque * 0.9F, 0.001F);

    const boost::shared_ptr<v3d::ui::style::Theme> derived = boost::make_shared<v3d::ui::style::Theme>("derived");
    const boost::shared_ptr<v3d::ui::style::Style> accented = boost::make_shared<v3d::ui::style::Style>(
        v3d::ui::style::Selector{.name = "default", .className = "ui"});
    accented->addProperty(boost::make_shared<v3d::ui::style::property::Number>("panel-opacity", 0.5F), "number");
    accented->addProperty(boost::make_shared<v3d::ui::style::property::Color>("accent", glm::vec4(1.0F, 0.5F, 0.0F, 1.0F)), "color");
    derived->addStyle(accented);
    drawn.theme(derived);
    BOOST_CHECK_CLOSE(drawn.dressing().panel.a, 0.5F, 0.001F);

    drawn.theme(boost::shared_ptr<v3d::ui::style::Theme>());
    BOOST_CHECK_CLOSE(drawn.dressing().panel.a, opaque, 0.001F);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
};  // namespace v3d::ui::documents

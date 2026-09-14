/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ui/Immediate.h>
#include <api/ui/style/Resolver.h>
#include <api/ui/style/Style.h>
#include <api/ui/style/Theme.h>
#include <api/ui/style/property/Color.h>
#include <api/ui/style/property/Number.h>

#include <string>
#include <string_view>

#include <boost/test/unit_test.hpp>

#include <boost/make_shared.hpp>

namespace {

using v3d::ui::style::Resolver;

boost::shared_ptr<v3d::ui::style::Style> style(const std::string& name, const std::string& className) {
    return boost::make_shared<v3d::ui::style::Style>(name, className);
}

void colour(const boost::shared_ptr<v3d::ui::style::Style>& target, const std::string& name,
    const glm::vec4& value) {
    target->addProperty(boost::make_shared<v3d::ui::style::property::Color>(name, value), "color");
}

void metric(const boost::shared_ptr<v3d::ui::style::Style>& target, const std::string& name, float value) {
    target->addProperty(boost::make_shared<v3d::ui::style::property::Number>(name, value), "number");
}

constexpr glm::vec4 red(1.0f, 0.0f, 0.0f, 1.0f);
constexpr glm::vec4 green(0.0f, 1.0f, 0.0f, 1.0f);

};  // namespace

BOOST_AUTO_TEST_SUITE(resolver_test)

/**
 * With no theme at all, every class resolves to the base - which is what a theme carrying
 * nothing has to leave unchanged, per ADR-0020.
 **/
BOOST_AUTO_TEST_CASE(no_theme_resolves_to_the_base) {
    Resolver resolver;
    resolver.base().panel = red;

    const v3d::ui::paint::Dressing& dressing = resolver.resolve(Resolver::Class::Panel, std::string_view());
    BOOST_CHECK(dressing.panel == red);
}

/**
 * A class's own style is applied over the base, and what it does not name keeps the value it
 * had.
 **/
BOOST_AUTO_TEST_CASE(a_class_style_is_applied_over_the_base) {
    const boost::shared_ptr<v3d::ui::style::Theme> theme =
        boost::make_shared<v3d::ui::style::Theme>("dark");
    const boost::shared_ptr<v3d::ui::style::Style> panel = style("plate", "panel");
    colour(panel, "background", green);
    metric(panel, "radius", 6.0f);
    theme->addStyle(panel);

    Resolver resolver;
    resolver.base().panel = red;
    resolver.base().border = red;
    resolver.base().radius = 0.0f;
    resolver.theme(theme);

    const v3d::ui::paint::Dressing& dressing = resolver.resolve(Resolver::Class::Panel, "plate");
    BOOST_CHECK(dressing.panel == green);
    BOOST_CHECK_CLOSE(dressing.radius, 6.0f, 0.001f);
    // named nothing, so unchanged
    BOOST_CHECK(dressing.border == red);
}

/**
 * A component naming no style is dressed by whichever style of its class the theme holds
 * first, so a theme can dress every panel without every panel naming one.
 **/
BOOST_AUTO_TEST_CASE(a_component_naming_no_style_takes_the_first_of_its_class) {
    const boost::shared_ptr<v3d::ui::style::Theme> theme =
        boost::make_shared<v3d::ui::style::Theme>("dark");
    const boost::shared_ptr<v3d::ui::style::Style> panel = style("first", "panel");
    colour(panel, "background", green);
    theme->addStyle(panel);

    Resolver resolver;
    resolver.theme(theme);
    BOOST_CHECK(resolver.resolve(Resolver::Class::Panel, std::string_view()).panel == green);
}

/**
 * The answer is worked out once and kept, so the same ask comes back as the same object
 * rather than as a fresh one. This is the whole point of the class: a component asks every
 * frame and the theme changes almost never.
 **/
BOOST_AUTO_TEST_CASE(the_same_ask_is_worked_out_once) {
    Resolver resolver;
    const v3d::ui::paint::Dressing& first = resolver.resolve(Resolver::Class::Panel, "plate");
    const v3d::ui::paint::Dressing& second = resolver.resolve(Resolver::Class::Panel, "plate");
    BOOST_CHECK_EQUAL(&first, &second);
}

/**
 * A new theme drops what was resolved from the last one, or a ui would keep drawing in the
 * theme it was told to stop using.
 **/
BOOST_AUTO_TEST_CASE(a_new_theme_drops_what_was_resolved) {
    const boost::shared_ptr<v3d::ui::style::Theme> dark =
        boost::make_shared<v3d::ui::style::Theme>("dark");
    const boost::shared_ptr<v3d::ui::style::Style> darkPanel = style("plate", "panel");
    colour(darkPanel, "background", green);
    dark->addStyle(darkPanel);

    const boost::shared_ptr<v3d::ui::style::Theme> light =
        boost::make_shared<v3d::ui::style::Theme>("light");
    const boost::shared_ptr<v3d::ui::style::Style> lightPanel = style("plate", "panel");
    colour(lightPanel, "background", red);
    light->addStyle(lightPanel);

    Resolver resolver;
    resolver.theme(dark);
    BOOST_CHECK(resolver.resolve(Resolver::Class::Panel, "plate").panel == green);

    resolver.theme(light);
    BOOST_CHECK(resolver.resolve(Resolver::Class::Panel, "plate").panel == red);
}

/**
 * Taking the base to write drops what was resolved from it, because every answer was worked
 * out over the values about to change.
 **/
BOOST_AUTO_TEST_CASE(changing_the_base_drops_what_was_resolved) {
    Resolver resolver;
    resolver.base().panel = red;
    BOOST_CHECK(resolver.resolve(Resolver::Class::Panel, std::string_view()).panel == red);

    resolver.base().panel = green;
    BOOST_CHECK(resolver.resolve(Resolver::Class::Panel, std::string_view()).panel == green);
}

/**
 * A theme's "ui" style is what the base is read from, and it reaches every class through it.
 **/
BOOST_AUTO_TEST_CASE(the_ui_style_is_read_into_the_base) {
    const boost::shared_ptr<v3d::ui::style::Theme> theme =
        boost::make_shared<v3d::ui::style::Theme>("dark");
    const boost::shared_ptr<v3d::ui::style::Style> chrome = style("default", "ui");
    colour(chrome, "border", green);
    metric(chrome, "border-width", 3.0f);
    theme->addStyle(chrome);

    Resolver resolver;
    resolver.theme(theme);

    BOOST_CHECK(resolver.base().border == green);
    // and a class that names no border of its own draws in it
    const v3d::ui::paint::Dressing& dressing = resolver.resolve(Resolver::Class::Bar, std::string_view());
    BOOST_CHECK(dressing.border == green);
    BOOST_CHECK_CLOSE(dressing.borderWidth, 3.0f, 0.001f);
}

/**
 * The colour a control that cannot be used is drawn in is the theme's to name, and it is
 * named once in "ui" rather than per class - so a theme that dresses one control disabled
 * has dressed them all. ADR-0059.
 **/
BOOST_AUTO_TEST_CASE(the_ui_style_names_the_disabled_colour) {
    const boost::shared_ptr<v3d::ui::style::Theme> theme =
        boost::make_shared<v3d::ui::style::Theme>("dark");
    const boost::shared_ptr<v3d::ui::style::Style> chrome = style("default", "ui");
    colour(chrome, "disabled-text", green);
    theme->addStyle(chrome);

    Resolver resolver;
    resolver.base().disabledText = red;
    resolver.theme(theme);

    BOOST_CHECK(resolver.base().disabledText == green);
    const v3d::ui::paint::Dressing& dressing = resolver.resolve(Resolver::Class::Button, std::string_view());
    BOOST_CHECK(dressing.disabledText == green);
}

/**
 * A scrollbar is not a progress bar. They read the same property names out of different
 * classes, so a theme that paints a health bar green leaves a scrollbar alone.
 **/
BOOST_AUTO_TEST_CASE(a_scrollbar_does_not_take_a_bars_style) {
    const boost::shared_ptr<v3d::ui::style::Theme> theme =
        boost::make_shared<v3d::ui::style::Theme>("dark");
    const boost::shared_ptr<v3d::ui::style::Style> bar = style("health", "bar");
    colour(bar, "track", green);
    theme->addStyle(bar);

    Resolver resolver;
    resolver.base().track = red;
    resolver.theme(theme);

    BOOST_CHECK(resolver.resolve(Resolver::Class::Bar, "health").track == green);
    BOOST_CHECK(resolver.resolve(Resolver::Class::Scrollbar, "health").track == red);
}

/**
 * A class rings its own control, so a theme can mark a focused text box differently from a
 * focused list - and a class naming neither keeps the base's ring, which is the one ring every
 * control showed before a class could ask for its own.
 **/
BOOST_AUTO_TEST_CASE(a_class_can_ring_its_own_control) {
    const boost::shared_ptr<v3d::ui::style::Theme> theme =
        boost::make_shared<v3d::ui::style::Theme>("dark");
    const boost::shared_ptr<v3d::ui::style::Style> field = style("default", "textbox");
    colour(field, "focus", green);
    metric(field, "focus-width", 4.0f);
    theme->addStyle(field);
    // named so that the class is found, and naming no ring of its own
    theme->addStyle(style("default", "list"));

    Resolver resolver;
    resolver.base().focus = red;
    resolver.base().focusWidth = 1.0f;
    resolver.theme(theme);

    const v3d::ui::paint::Dressing& box = resolver.resolve(Resolver::Class::TextBox, std::string_view());
    BOOST_CHECK(box.focus == green);
    BOOST_CHECK_CLOSE(box.focusWidth, 4.0f, 0.001f);

    const v3d::ui::paint::Dressing& list = resolver.resolve(Resolver::Class::List, std::string_view());
    BOOST_CHECK(list.focus == red);
    BOOST_CHECK_CLOSE(list.focusWidth, 1.0f, 0.001f);
}

/**
 * A button's ring comes out of the "button" class, which is the only thing a button reads as a
 * Dressing - its fill is nine images and its label is the base's.
 *
 * A button's styles are told apart by state as well as by name, and the first of the set is
 * what answers here, because a ring says where the keyboard is rather than what state the
 * button is in.
 **/
BOOST_AUTO_TEST_CASE(a_buttons_ring_comes_out_of_the_button_class) {
    const boost::shared_ptr<v3d::ui::style::Theme> theme =
        boost::make_shared<v3d::ui::style::Theme>("dark");
    const boost::shared_ptr<v3d::ui::style::Style> pressed = style("default", "button");
    colour(pressed, "focus", green);
    theme->addStyle(pressed);

    Resolver resolver;
    resolver.base().focus = red;
    resolver.theme(theme);

    BOOST_CHECK(resolver.resolve(Resolver::Class::Button, std::string_view()).focus == green);
    // and a class the theme says nothing about is still ringed out of the base
    BOOST_CHECK(resolver.resolve(Resolver::Class::Tabs, std::string_view()).focus == red);
}

/**
 * The immediate layer reads its own style class, not the retained side's.
 *
 * The two want the same keys at different sizes - a hud is read at a glance and a tool panel
 * is read closely - so a theme that set "line-height" for one used to break the other.
 **/
BOOST_AUTO_TEST_CASE(the_two_ways_of_writing_a_ui_read_different_classes) {
    const boost::shared_ptr<v3d::ui::style::Theme> theme =
        boost::make_shared<v3d::ui::style::Theme>("dark");
    const boost::shared_ptr<v3d::ui::style::Style> chrome = style("default", Resolver::chromeClass);
    metric(chrome, "line-height", 34.0f);
    theme->addStyle(chrome);
    const boost::shared_ptr<v3d::ui::style::Style> tools = style("default", Resolver::tools);
    metric(tools, "line-height", 18.0f);
    theme->addStyle(tools);

    Resolver resolver;
    resolver.theme(theme);
    BOOST_CHECK_CLOSE(resolver.base().lineHeight, 34.0f, 0.001f);

    v3d::ui::Immediate ui(
        [](std::string_view text) { return static_cast<float>(text.size()) * 10.0f; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});
    ui.theme(theme);
    BOOST_CHECK_CLOSE(ui.dressing().lineHeight, 18.0f, 0.001f);
}

/**
 * A theme naming only the retained side's class leaves the immediate layer in its own
 * defaults, rather than dressing it in metrics meant for a hud.
 **/
BOOST_AUTO_TEST_CASE(a_theme_that_dresses_one_side_leaves_the_other_alone) {
    const boost::shared_ptr<v3d::ui::style::Theme> theme =
        boost::make_shared<v3d::ui::style::Theme>("dark");
    const boost::shared_ptr<v3d::ui::style::Style> chrome = style("default", Resolver::chromeClass);
    metric(chrome, "line-height", 34.0f);
    theme->addStyle(chrome);

    v3d::ui::Immediate ui(
        [](std::string_view text) { return static_cast<float>(text.size()) * 10.0f; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});
    const float before = ui.dressing().lineHeight;
    ui.theme(theme);
    BOOST_CHECK_CLOSE(ui.dressing().lineHeight, before, 0.001f);
}

BOOST_AUTO_TEST_SUITE_END()

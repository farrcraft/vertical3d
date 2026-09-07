/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>
#include <string_view>

#include <boost/test/unit_test.hpp>

#include "../style/Resolver.h"

#include "../Style.h"
#include "../style/Theme.h"
#include "../style/property/Color.h"
#include "../style/property/Number.h"

#include <boost/make_shared.hpp>

namespace {

using v3d::ui::style::Resolver;

boost::shared_ptr<v3d::ui::Style> style(const std::string& name, const std::string& className) {
    return boost::make_shared<v3d::ui::Style>(name, className);
}

void colour(const boost::shared_ptr<v3d::ui::Style>& target, const std::string& name,
    const glm::vec4& value) {
    target->addProperty(boost::make_shared<v3d::ui::style::property::Color>(name, value), "color");
}

void metric(const boost::shared_ptr<v3d::ui::Style>& target, const std::string& name, float value) {
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

    const v3d::ui::Dressing& dressing = resolver.resolve(Resolver::Class::Panel, std::string_view());
    BOOST_CHECK(dressing.panel == red);
}

/**
 * A class's own style is applied over the base, and what it does not name keeps the value it
 * had.
 **/
BOOST_AUTO_TEST_CASE(a_class_style_is_applied_over_the_base) {
    const boost::shared_ptr<v3d::ui::style::Theme> theme =
        boost::make_shared<v3d::ui::style::Theme>("dark");
    const boost::shared_ptr<v3d::ui::Style> panel = style("plate", "panel");
    colour(panel, "background", green);
    metric(panel, "radius", 6.0f);
    theme->addStyle(panel);

    Resolver resolver;
    resolver.base().panel = red;
    resolver.base().border = red;
    resolver.base().radius = 0.0f;
    resolver.theme(theme);

    const v3d::ui::Dressing& dressing = resolver.resolve(Resolver::Class::Panel, "plate");
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
    const boost::shared_ptr<v3d::ui::Style> panel = style("first", "panel");
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
    const v3d::ui::Dressing& first = resolver.resolve(Resolver::Class::Panel, "plate");
    const v3d::ui::Dressing& second = resolver.resolve(Resolver::Class::Panel, "plate");
    BOOST_CHECK_EQUAL(&first, &second);
}

/**
 * A new theme drops what was resolved from the last one, or a ui would keep drawing in the
 * theme it was told to stop using.
 **/
BOOST_AUTO_TEST_CASE(a_new_theme_drops_what_was_resolved) {
    const boost::shared_ptr<v3d::ui::style::Theme> dark =
        boost::make_shared<v3d::ui::style::Theme>("dark");
    const boost::shared_ptr<v3d::ui::Style> darkPanel = style("plate", "panel");
    colour(darkPanel, "background", green);
    dark->addStyle(darkPanel);

    const boost::shared_ptr<v3d::ui::style::Theme> light =
        boost::make_shared<v3d::ui::style::Theme>("light");
    const boost::shared_ptr<v3d::ui::Style> lightPanel = style("plate", "panel");
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
    const boost::shared_ptr<v3d::ui::Style> chrome = style("default", "ui");
    colour(chrome, "border", green);
    metric(chrome, "border-width", 3.0f);
    theme->addStyle(chrome);

    Resolver resolver;
    resolver.theme(theme);

    BOOST_CHECK(resolver.base().border == green);
    // and a class that names no border of its own draws in it
    const v3d::ui::Dressing& dressing = resolver.resolve(Resolver::Class::Bar, std::string_view());
    BOOST_CHECK(dressing.border == green);
    BOOST_CHECK_CLOSE(dressing.borderWidth, 3.0f, 0.001f);
}

/**
 * A scrollbar is not a progress bar. They read the same property names out of different
 * classes, so a theme that paints a health bar green leaves a scrollbar alone.
 **/
BOOST_AUTO_TEST_CASE(a_scrollbar_does_not_take_a_bars_style) {
    const boost::shared_ptr<v3d::ui::style::Theme> theme =
        boost::make_shared<v3d::ui::style::Theme>("dark");
    const boost::shared_ptr<v3d::ui::Style> bar = style("health", "bar");
    colour(bar, "track", green);
    theme->addStyle(bar);

    Resolver resolver;
    resolver.base().track = red;
    resolver.theme(theme);

    BOOST_CHECK(resolver.resolve(Resolver::Class::Bar, "health").track == green);
    BOOST_CHECK(resolver.resolve(Resolver::Class::Scrollbar, "health").track == red);
}

BOOST_AUTO_TEST_SUITE_END()

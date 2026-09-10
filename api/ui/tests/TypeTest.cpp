/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ui/component/Type.h>

#include <set>
#include <string>
#include <string_view>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * How far past the enum the sweeps below run.
 *
 * component::parse() walks to VerticalBox because the enum is kept alphabetical and that is
 * its last entry. Sweeping wider is what turns that into something checked rather than
 * assumed: a type added past VerticalBox is named by name(), whose switch the compiler makes
 * exhaustive, and is then found here while parse() would have skipped it.
 *
 * An int outside the enumerators is a valid value of a scoped enum's underlying type, so
 * casting one and switching on it falls through to name()'s empty answer rather than being
 * undefined.
 **/
const int beyond = 64;

};  // namespace

BOOST_AUTO_TEST_SUITE(type_test)

/**
 * Every type a config can name parses back to itself, per ADR-0047.
 *
 * The two halves of the config's vocabulary have to agree or a component is loadable under a
 * name nothing spells: name() is exhaustive and so cannot forget a type, and this is what
 * stops parse() forgetting one.
 **/
BOOST_AUTO_TEST_CASE(every_named_type_parses_back_to_itself) {
    int named = 0;
    for (int index = 0; index < beyond; ++index) {
        const v3d::ui::component::Type type = static_cast<v3d::ui::component::Type>(index);
        const std::string_view text = v3d::ui::component::name(type);
        if (text.empty()) {
            continue;
        }
        ++named;
        BOOST_CHECK_MESSAGE(v3d::ui::component::parse(text) == type,
            "type " << index << " is named \"" << text << "\" and does not parse back to itself");
    }
    // the seventeen a config can ask for: everything in the enum but MenuItem, which the menu
    // holding it builds, and Undefined, which is not a component
    BOOST_CHECK_EQUAL(named, 17);
}

/**
 * No two types answer to the same name, or one of them is unreachable from a config.
 **/
BOOST_AUTO_TEST_CASE(no_two_types_share_a_name) {
    std::set<std::string> seen;
    for (int index = 0; index < beyond; ++index) {
        const std::string_view text =
            v3d::ui::component::name(static_cast<v3d::ui::component::Type>(index));
        if (text.empty()) {
            continue;
        }
        BOOST_CHECK_MESSAGE(seen.insert(std::string(text)).second,
            "the name \"" << text << "\" is answered by more than one type");
    }
}

/**
 * A name no component answers to is Undefined rather than a guess, which is what the loader
 * reports as an unrecognised type.
 **/
BOOST_AUTO_TEST_CASE(an_unknown_name_is_undefined) {
    BOOST_CHECK(v3d::ui::component::parse("") == v3d::ui::component::Type::Undefined);
    BOOST_CHECK(v3d::ui::component::parse("widget") == v3d::ui::component::Type::Undefined);
    // a type that exists and that a config cannot name is still not parseable
    BOOST_CHECK(v3d::ui::component::parse("menuitem") == v3d::ui::component::Type::Undefined);
    BOOST_CHECK(v3d::ui::component::name(v3d::ui::component::Type::MenuItem).empty());
}

BOOST_AUTO_TEST_SUITE_END()

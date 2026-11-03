/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vertical3d/src/script/Arguments.h>
#include <vertical3d/src/script/Journal.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <boost/json/parse.hpp>
#include <boost/test/unit_test.hpp>

namespace {

using v3d::editor::script::Arguments;
using v3d::editor::script::Journal;
using v3d::editor::script::Parameter;
using v3d::editor::script::Type;

/**
 * @return the parameters of a command that takes one of each type
 **/
std::vector<Parameter> each() {
    return { { "on", Type::Boolean }, { "amount", Type::Number }, { "frame", Type::Integer }, { "label", Type::Text },
        { "item", Type::Item }, { "items", Type::Items }, { "parts", Type::Parts } };
}

/**
 * @return why the arguments in a JSON text do not suit each()
 **/
std::optional<std::string> checked(const char* text) {
    return Arguments::check(boost::json::parse(text).as_object(), each());
}

BOOST_AUTO_TEST_SUITE(script_arguments_test)

/**
 * A value of each type passes, and so does nothing at all.
 **/
BOOST_AUTO_TEST_CASE(values_of_their_types_pass) {
    BOOST_CHECK(!checked("{}"));
    BOOST_CHECK(!checked(R"({"on": true, "amount": 0.5, "frame": 24, "label": "a", "item": "Cube", "items": ["Cube"], "parts": ["Cube.f[0]"]})"));
    BOOST_CHECK(!checked(R"({"frame": -2147483648})"));
}

/**
 * A name no parameter has is refused, and so is a value of the wrong type, each saying which.
 **/
BOOST_AUTO_TEST_CASE(a_stranger_or_a_wrong_type_is_refused) {
    BOOST_CHECK_EQUAL(checked(R"({"colour": 1})").value_or(""), "it takes no argument called colour");
    BOOST_CHECK_EQUAL(checked(R"({"on": 1})").value_or(""), "on is true or false");
    BOOST_CHECK_EQUAL(checked(R"({"amount": "1"})").value_or(""), "amount is a number");
    BOOST_CHECK_EQUAL(checked(R"({"frame": 2.5})").value_or(""), "frame is a whole number");
    // a number with a point is not an integer, even a whole one, and nor is one an int cannot hold
    BOOST_CHECK_EQUAL(checked(R"({"frame": 24.0})").value_or(""), "frame is a whole number");
    BOOST_CHECK_EQUAL(checked(R"({"frame": 2147483648})").value_or(""), "frame is a whole number");
    BOOST_CHECK_EQUAL(checked(R"({"label": false})").value_or(""), "label is text");
    BOOST_CHECK_EQUAL(checked(R"({"item": ["Cube"]})").value_or(""), "item is an item's name");
    BOOST_CHECK_EQUAL(checked(R"({"items": "Cube"})").value_or(""), "items is a list of item names");
    BOOST_CHECK_EQUAL(checked(R"({"parts": ["Cube.f[0]", 2]})").value_or(""), "parts is a list of parts");
}

/**
 * A value given is read as it was given, and the fill is never asked.
 **/
BOOST_AUTO_TEST_CASE(a_value_given_is_read) {
    Arguments arguments(boost::json::parse(R"({"on": true, "amount": 2, "frame": 12, "label": "a", "items": ["Cube", "Cone"]})").as_object());
    const auto never = []() -> bool {
        BOOST_ERROR("a given value was filled");
        return false;
    };
    BOOST_CHECK(arguments.boolean("on", never));
    BOOST_CHECK_EQUAL(arguments.number("amount", []() { return 0.0; }), 2.0);
    BOOST_CHECK_EQUAL(arguments.integer("frame", []() { return 0; }), 12);
    BOOST_CHECK_EQUAL(arguments.text("label", []() { return std::string(); }), "a");
    const std::vector<std::string> items = arguments.names("items", []() { return std::vector<std::string>(); });
    BOOST_CHECK(items == (std::vector<std::string>{ "Cube", "Cone" }));
}

/**
 * A value left out is filled once, and kept as if it had been given, so the line written for
 * the run holds it.
 **/
BOOST_AUTO_TEST_CASE(a_value_left_out_is_filled_and_kept) {
    Arguments arguments;
    int asked = 0;
    BOOST_CHECK(!arguments.has("frame"));
    BOOST_CHECK_EQUAL(arguments.integer("frame", [&asked]() { asked++; return 24; }), 24);
    BOOST_CHECK_EQUAL(arguments.integer("frame", [&asked]() { asked++; return 1; }), 24);
    BOOST_CHECK_EQUAL(asked, 1);
    BOOST_CHECK(arguments.has("frame"));
    BOOST_CHECK(arguments.names("items", []() { return std::vector<std::string>{ "Cube" }; }) == std::vector<std::string>{ "Cube" });
    BOOST_CHECK_EQUAL(boost::json::serialize(arguments.values()), R"({"frame":24,"items":["Cube"]})");
}

/**
 * The journal keeps its entries in order, numbered from 1, and drops the oldest past its
 * capacity. An entry is written again by its number while it is kept.
 **/
BOOST_AUTO_TEST_CASE(the_journal_keeps_the_latest_entries) {
    Journal journal(2);
    BOOST_CHECK_EQUAL(journal.add(Journal::Kind::Command, "edit.undo()"), 1U);
    BOOST_CHECK_EQUAL(journal.add(Journal::Kind::Refused, "nothing to undo"), 2U);
    const std::uint64_t was = journal.revision();
    journal.rewrite(1, "edit.redo()");
    BOOST_CHECK_EQUAL(journal.entries().front().text, "edit.redo()");
    BOOST_CHECK_GT(journal.revision(), was);

    BOOST_CHECK_EQUAL(journal.add(Journal::Kind::Done, "created a cube"), 3U);
    BOOST_REQUIRE_EQUAL(journal.entries().size(), 2U);
    BOOST_CHECK_EQUAL(journal.entries().front().number, 2U);
    BOOST_CHECK(journal.entries().front().kind == Journal::Kind::Refused);
    BOOST_CHECK_EQUAL(journal.entries().back().text, "created a cube");

    // an entry dropped is not written, and no other entry is written in its place
    journal.rewrite(1, "lost");
    BOOST_CHECK_EQUAL(journal.entries().front().text, "nothing to undo");
    journal.rewrite(9, "lost");
    BOOST_CHECK_EQUAL(journal.entries().back().text, "created a cube");
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

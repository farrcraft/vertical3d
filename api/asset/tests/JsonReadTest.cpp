/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Json.h>

#include <string>

#include <boost/json.hpp>
#include <boost/test/unit_test.hpp>

using v3d::asset::readArray;
using v3d::asset::readBool;
using v3d::asset::readNumber;
using v3d::asset::readObject;
using v3d::asset::readString;

namespace {

boost::json::object sample() {
    return boost::json::parse(R"({
        "text": "acorn",
        "empty": "",
        "whole": 3,
        "large": 18446744073709551615,
        "fraction": 2.5,
        "yes": true,
        "nothing": null,
        "inner": { "a": 1 },
        "list": [1, 2, 3]
    })").as_object();
}

};  // namespace

BOOST_AUTO_TEST_SUITE(json_read_test)

BOOST_AUTO_TEST_CASE(a_string_member_is_read) {
    const boost::json::object object = sample();
    BOOST_CHECK(readString(object, "text") == "acorn");
    BOOST_CHECK(readString(object, "empty") == "");
}

BOOST_AUTO_TEST_CASE(a_string_read_refuses_a_missing_member_and_every_other_type) {
    const boost::json::object object = sample();
    BOOST_CHECK(!readString(object, "missing").has_value());
    for (const char* key : {"whole", "fraction", "yes", "nothing", "inner", "list"}) {
        BOOST_TEST_CONTEXT(key) {
            BOOST_CHECK_NO_THROW(BOOST_CHECK(!readString(object, key).has_value()));
        }
    }
}

BOOST_AUTO_TEST_CASE(a_number_member_of_any_kind_is_read_as_a_double) {
    const boost::json::object object = sample();
    BOOST_CHECK(readNumber(object, "whole") == 3.0);
    BOOST_CHECK(readNumber(object, "fraction") == 2.5);
    BOOST_CHECK_CLOSE(readNumber(object, "large").value_or(0.0), 18446744073709551615.0, 1e-9);
}

BOOST_AUTO_TEST_CASE(a_number_read_refuses_a_missing_member_and_every_other_type) {
    const boost::json::object object = sample();
    BOOST_CHECK(!readNumber(object, "missing").has_value());
    for (const char* key : {"text", "yes", "nothing", "inner", "list"}) {
        BOOST_TEST_CONTEXT(key) {
            BOOST_CHECK_NO_THROW(BOOST_CHECK(!readNumber(object, key).has_value()));
        }
    }
}

BOOST_AUTO_TEST_CASE(a_bool_read_takes_only_true_or_false) {
    const boost::json::object object = sample();
    BOOST_CHECK(readBool(object, "yes") == true);
    BOOST_CHECK(!readBool(object, "missing").has_value());
    // a number is not taken as a bool
    for (const char* key : {"text", "whole", "nothing", "inner", "list"}) {
        BOOST_TEST_CONTEXT(key) {
            BOOST_CHECK_NO_THROW(BOOST_CHECK(!readBool(object, key).has_value()));
        }
    }
}

BOOST_AUTO_TEST_CASE(an_object_read_points_into_the_object) {
    const boost::json::object object = sample();
    const boost::json::object* inner = readObject(object, "inner");
    BOOST_REQUIRE(inner != nullptr);
    BOOST_CHECK_EQUAL(inner, &object.at("inner").as_object());
    BOOST_CHECK(readObject(object, "missing") == nullptr);
    BOOST_CHECK(readObject(object, "list") == nullptr);
    BOOST_CHECK(readObject(object, "text") == nullptr);
}

BOOST_AUTO_TEST_CASE(an_array_read_points_into_the_object) {
    const boost::json::object object = sample();
    const boost::json::array* list = readArray(object, "list");
    BOOST_REQUIRE(list != nullptr);
    BOOST_CHECK_EQUAL(list->size(), 3u);
    BOOST_CHECK_EQUAL(list, &object.at("list").as_array());
    BOOST_CHECK(readArray(object, "missing") == nullptr);
    BOOST_CHECK(readArray(object, "inner") == nullptr);
    BOOST_CHECK(readArray(object, "nothing") == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()

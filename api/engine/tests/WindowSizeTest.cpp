/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/WindowSize.h>

#include <optional>
#include <string>

#include <boost/json.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/vec2.hpp>

namespace {

std::optional<glm::ivec2> read(const char* document) {
    return v3d::engine::windowSize(boost::json::parse(document).as_object());
}

BOOST_AUTO_TEST_CASE(a_window_document_gives_its_size_test) {
    BOOST_CHECK(read(R"({ "window": { "width": 800, "height": 600 } })") == glm::ivec2(800, 600));
}

/**
 * A dimension an int cannot hold is refused rather than narrowed, which would make 2^32 + 640
 * a window 640 wide. A fraction, a string and a missing window are refused too.
 **/
BOOST_AUTO_TEST_CASE(a_window_size_an_int_cannot_hold_is_refused_test) {
    for (const char* document : {
            R"({ "window": { "width": 4294967936, "height": 600 } })",
            R"({ "window": { "width": 800, "height": -4294966696 } })",
            R"({ "window": { "width": 800.5, "height": 600 } })",
            R"({ "window": { "width": "800", "height": 600 } })",
            R"({ "window": { "width": 800 } })",
            R"({ "size": { "width": 800, "height": 600 } })" }) {
        BOOST_TEST_CONTEXT(document) {
            BOOST_CHECK(!read(document).has_value());
        }
    }
}

/**
 * A window document may name the SVG its icon is drawn from. One that names none, names it as
 * something other than a string, or has no window, gives no icon.
 **/
BOOST_AUTO_TEST_CASE(a_window_document_names_its_icon_test) {
    const auto icon = [](const char* document) { return v3d::engine::windowIcon(boost::json::parse(document).as_object()); };
    BOOST_CHECK(icon(R"({ "window": { "width": 800, "height": 600, "icon": "icon.svg" } })") == std::string("icon.svg"));
    BOOST_CHECK(!icon(R"({ "window": { "width": 800, "height": 600 } })"));
    BOOST_CHECK(!icon(R"({ "window": { "icon": 3 } })"));
    BOOST_CHECK(!icon(R"({ "icon": "icon.svg" })"));
}

};  // namespace

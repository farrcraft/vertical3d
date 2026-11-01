/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ui/Image.h>
#include <vertical3d/src/control/IconSheet.h>

#include <cstddef>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

#include <boost/json/parse.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>

namespace {

using v3d::editor::control::IconSheet;

BOOST_AUTO_TEST_SUITE(icon_sheet_test)

/**
 * A cell's corners follow from its place, row by row, with as many rows as the names fill.
 **/
BOOST_AUTO_TEST_CASE(a_cell_is_found_by_its_name) {
    IconSheet sheet;
    BOOST_REQUIRE(sheet.load(boost::json::parse(R"({"image": "icons/sheet.png", "columns": 2, "names": ["a", "b", "c"]})").as_object()));
    BOOST_CHECK_EQUAL(sheet.image(), "icons/sheet.png");

    const std::optional<v3d::ui::Image::Corners> first = sheet.corners("icon:a");
    BOOST_REQUIRE(first.has_value());
    BOOST_CHECK(first.value_or(v3d::ui::Image::Corners()).min == glm::vec2(0.0F, 0.0F));
    BOOST_CHECK(first.value_or(v3d::ui::Image::Corners()).max == glm::vec2(0.5F, 0.5F));
    const std::optional<v3d::ui::Image::Corners> third = sheet.corners("icon:c");
    BOOST_REQUIRE(third.has_value());
    BOOST_CHECK(third.value_or(v3d::ui::Image::Corners()).min == glm::vec2(0.0F, 0.5F));
    BOOST_CHECK(third.value_or(v3d::ui::Image::Corners()).max == glm::vec2(0.5F, 1.0F));

    BOOST_CHECK(!sheet.corners("icon:d"));
    BOOST_CHECK(!sheet.corners("a"));
    BOOST_CHECK(!sheet.corners("icons/sheet.png"));
}

/**
 * A sheet with no image, no columns above zero or no names is refused, and the one held stays.
 **/
BOOST_AUTO_TEST_CASE(a_sheet_that_cannot_be_read_is_refused) {
    IconSheet sheet;
    BOOST_REQUIRE(sheet.load(boost::json::parse(R"({"image": "kept.png", "columns": 1, "names": ["a"]})").as_object()));
    for (const char* text : { R"({"columns": 1, "names": ["a"]})", R"({"image": "x.png", "columns": 0, "names": ["a"]})",
             R"({"image": "x.png", "columns": 1, "names": []})", R"({"image": "x.png", "columns": 1, "names": [1]})" }) {
        BOOST_CHECK_MESSAGE(!sheet.load(boost::json::parse(text).as_object()), text);
    }
    BOOST_CHECK_EQUAL(sheet.image(), "kept.png");
    BOOST_CHECK(sheet.corners("icon:a").has_value());
}

/**
 * The sheet the editor ships names every icon its ui config draws.
 **/
BOOST_AUTO_TEST_CASE(the_shipped_sheet_has_every_icon_the_ui_names) {
    const std::ifstream file("data/icons.json");
    BOOST_REQUIRE(file.is_open());
    std::stringstream text;
    text << file.rdbuf();
    IconSheet sheet;
    BOOST_REQUIRE(sheet.load(boost::json::parse(text.str()).as_object()));

    const std::ifstream ui("data/vgui.json");
    BOOST_REQUIRE(ui.is_open());
    std::stringstream config;
    config << ui.rdbuf();
    const std::string all = config.str();
    int named = 0;
    for (std::size_t at = all.find("\"icon:"); at != std::string::npos; at = all.find("\"icon:", at + 1)) {
        const std::string source = all.substr(at + 1, all.find('"', at + 1) - at - 1);
        BOOST_CHECK_MESSAGE(sheet.corners(source).has_value(), source);
        named++;
    }
    BOOST_CHECK_GE(named, 9);
    BOOST_CHECK(sheet.corners("icon:maximise").has_value());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

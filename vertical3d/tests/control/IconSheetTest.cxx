/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/File.h>
#include <api/image/Image.h>
#include <api/image/reader/Svg.h>
#include <api/log/Logger.h>
#include <api/ui/Image.h>
#include <vertical3d/src/control/IconSheet.h>

#include <cstddef>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>

namespace {

using v3d::editor::control::IconSheet;

/**
 * A white square that fills its document.
 **/
const char square[] = R"(<svg xmlns="http://www.w3.org/2000/svg" width="4" height="4"><rect width="4" height="4" fill="#ffffff"/></svg>)";

/**
 * @return the alpha of the pixel at x and y
 **/
unsigned int alpha(const v3d::image::Image& image, unsigned int x, unsigned int y) {
    return image[(((y * image.width()) + x) * 4) + 3];
}

BOOST_AUTO_TEST_SUITE(icon_sheet_test)

/**
 * The cells are laid out row by row in a grid as many columns wide as the square root of the
 * count, rounded up, and a cell's corners follow from its place.
 **/
BOOST_AUTO_TEST_CASE(a_cell_is_found_by_its_name) {
    IconSheet sheet;
    BOOST_REQUIRE(sheet.load(boost::json::parse(R"({"directory": "icons", "names": ["a", "b", "c"]})").as_object()));
    BOOST_CHECK_EQUAL(sheet.columns(), 2U);
    BOOST_CHECK_EQUAL(sheet.rows(), 2U);
    BOOST_CHECK_EQUAL(sheet.file("b"), "icons/b.svg");

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
}

/**
 * A sheet with no directory or no names is refused, and the one held stays.
 **/
BOOST_AUTO_TEST_CASE(a_sheet_that_cannot_be_read_is_refused) {
    IconSheet sheet;
    BOOST_REQUIRE(sheet.load(boost::json::parse(R"({"directory": "kept", "names": ["a"]})").as_object()));
    for (const char* text : { R"({"names": ["a"]})", R"({"directory": "", "names": ["a"]})", R"({"directory": "x", "names": []})",
             R"({"directory": "x", "names": [1]})", R"({"directory": "x", "names": [""]})" }) {
        BOOST_CHECK_MESSAGE(!sheet.load(boost::json::parse(text).as_object()), text);
    }
    BOOST_CHECK_EQUAL(sheet.file("a"), "kept/a.svg");
    BOOST_CHECK(sheet.corners("icon:a").has_value());
}

/**
 * Each icon is drawn into its own cell at the side asked for. One whose file cannot be read
 * leaves its cell empty, and the others are still drawn.
 **/
BOOST_AUTO_TEST_CASE(each_icon_is_drawn_into_its_cell) {
    IconSheet sheet;
    BOOST_REQUIRE(sheet.load(boost::json::parse(R"({"directory": "icons", "names": ["a", "missing", "c"]})").as_object()));
    const IconSheet::Read read = [](const std::string& path) -> std::optional<std::string> {
        return path == "icons/missing.svg" ? std::nullopt : std::optional<std::string>(square);
    };
    const boost::shared_ptr<v3d::image::Image> drawn = sheet.draw(8, read, boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(drawn);
    BOOST_CHECK_EQUAL(drawn->width(), 16U);
    BOOST_CHECK_EQUAL(drawn->height(), 16U);
    BOOST_CHECK_EQUAL(alpha(*drawn, 4, 4), 255U);
    BOOST_CHECK_EQUAL(alpha(*drawn, 12, 4), 0U);
    BOOST_CHECK_EQUAL(alpha(*drawn, 4, 12), 255U);
    BOOST_CHECK_EQUAL(alpha(*drawn, 12, 12), 0U);
    BOOST_CHECK(!sheet.draw(0, read, boost::make_shared<v3d::log::Logger>()));
}

/**
 * The sheet the editor ships names every icon its ui config draws, and every icon it names is a
 * file that draws.
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

    const IconSheet::Read read = [](const std::string& path) { return v3d::asset::readFile("data/" + path); };
    v3d::image::reader::Svg svg(boost::make_shared<v3d::log::Logger>());
    for (const std::string& name : sheet.names()) {
        const std::optional<std::string> source = read(sheet.file(name));
        BOOST_CHECK_MESSAGE(source.has_value(), name);
        if (source) {
            BOOST_CHECK_MESSAGE(svg.read(reinterpret_cast<const unsigned char*>(source->data()), source->size(), glm::uvec2(18, 18)), name);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

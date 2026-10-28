/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Writer.h>
#include <api/asset/media/SpriteRegion.h>
#include <api/asset/media/SpriteSheet.h>
#include <api/asset/media/SpriteSheets.h>
#include <api/image/Image.h>
#include <api/image/Pack.h>
#include <api/log/Logger.h>

#include <cstddef>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

boost::shared_ptr<v3d::log::Logger> logger() {
    return boost::make_shared<v3d::log::Logger>();
}

v3d::image::Sprite sprite(const std::string& name, unsigned int width, unsigned int height) {
    return {.name = name, .image = boost::make_shared<v3d::image::Image>(glm::uvec2(width, height), 32)};
}

/**
 * A directory of its own per case, removed with everything under it.
 **/
class Sandbox {
 public:
    explicit Sandbox(const std::string& name) :
        path_(boost::filesystem::temp_directory_path() / ("v3d_sheets_" + name)) {
        boost::system::error_code ignored;
        boost::filesystem::remove_all(path_, ignored);
        boost::filesystem::create_directories(path_);
    }

    ~Sandbox() {
        boost::system::error_code ignored;
        boost::filesystem::remove_all(path_, ignored);
    }

    Sandbox(const Sandbox&) = delete;
    Sandbox& operator=(const Sandbox&) = delete;

    boost::filesystem::path file(const std::string& name) const {
        return path_ / name;
    }

 private:
    boost::filesystem::path path_;
};

/**
 * A packed sheet is the packed image's size, names the file it is written to, and holds each
 * placement as a sprite under its name, in the order the sprites were packed.
 **/
BOOST_AUTO_TEST_CASE(sprite_sheet_holds_what_was_packed_test) {
    const v3d::image::Packing packing = v3d::image::pack({sprite("tall", 8, 20), sprite("wide", 30, 6)}, 256, logger());
    BOOST_REQUIRE(packing.sheet);

    const v3d::asset::media::SpriteSheet sheet = v3d::asset::media::SpriteSheet::packed("units", "units.png", packing);
    BOOST_TEST(sheet.name() == "units");
    BOOST_TEST(sheet.image() == "units.png");
    BOOST_TEST(sheet.width() == static_cast<int>(packing.sheet->width()));
    BOOST_TEST(sheet.height() == static_cast<int>(packing.sheet->height()));
    BOOST_TEST(sheet.sprites() == std::vector<std::string>({"tall", "wide"}), boost::test_tools::per_element());
    for (const v3d::image::Placement& placement : packing.placements) {
        BOOST_TEST_CONTEXT(placement.name) {
            const v3d::asset::media::SpriteRegion region = sheet.get(placement.name);
            BOOST_TEST(region.x == static_cast<int>(placement.area.x));
            BOOST_TEST(region.y == static_cast<int>(placement.area.y));
            BOOST_TEST(region.width == static_cast<int>(placement.area.z));
            BOOST_TEST(region.height == static_cast<int>(placement.area.w));
        }
    }
}

/**
 * A packing that refused its sprites gives a sheet with no name, which no document holds.
 **/
BOOST_AUTO_TEST_CASE(sprite_sheet_of_a_refused_packing_is_empty_test) {
    const v3d::image::Packing packing = v3d::image::pack({sprite("huge", 64, 64)}, 16, logger());
    BOOST_REQUIRE(!packing.sheet);

    const v3d::asset::media::SpriteSheet sheet = v3d::asset::media::SpriteSheet::packed("units", "units.png", packing);
    BOOST_TEST(sheet.name().empty());
    BOOST_TEST(sheet.sprites().empty());
    v3d::asset::media::SpriteSheets sheets(logger());
    BOOST_TEST(!sheets.add(sheet));
}

/**
 * A sheet's placements are where its sprites were packed, in its order, so unpacking a sheet
 * cuts out the rectangles packing it filled.
 **/
BOOST_AUTO_TEST_CASE(sprite_sheet_placements_are_where_it_was_packed_test) {
    const v3d::image::Packing packing =
        v3d::image::pack({sprite("a", 5, 9), sprite("b", 12, 3), sprite("c", 7, 7)}, 256, logger());
    BOOST_REQUIRE(packing.sheet);

    const std::vector<v3d::image::Placement> placements =
        v3d::asset::media::SpriteSheet::packed("units", "units.png", packing).placements();
    BOOST_REQUIRE_EQUAL(placements.size(), packing.placements.size());
    for (std::size_t index = 0; index < placements.size(); index++) {
        BOOST_TEST_CONTEXT(packing.placements[index].name) {
            BOOST_TEST(placements[index].name == packing.placements[index].name);
            BOOST_TEST((placements[index].area == packing.placements[index].area));
        }
    }

    const v3d::image::Unpacking unpacking = v3d::image::unpack(*packing.sheet, placements);
    BOOST_TEST(unpacking.refusal.empty());
    BOOST_TEST(unpacking.sprites.size() == 3U);
}

/**
 * A document file that does not exist holds no sheets, so the first sheet of a new document
 * is read without an error.
 **/
BOOST_AUTO_TEST_CASE(sprite_sheets_read_a_missing_file_as_none_test) {
    const Sandbox sandbox("missing");
    v3d::asset::media::SpriteSheets sheets(logger());
    BOOST_TEST(sheets.read(sandbox.file("sprites.json")));
    BOOST_TEST(sheets.names().empty());
}

/**
 * Packing one sheet into a document keeps the document's other sheets. The sheet of the same
 * name is replaced in its place, and the others are written back as they were read.
 **/
BOOST_AUTO_TEST_CASE(sprite_sheets_read_then_add_keeps_the_other_sheets_test) {
    const Sandbox sandbox("merge");
    const boost::filesystem::path document = sandbox.file("sprites.json");
    {
        v3d::asset::media::SpriteSheets first(logger());
        v3d::asset::media::SpriteSheet terrain({.name = "terrain", .image = "terrain.png", .width = 64, .height = 64});
        v3d::asset::media::SpriteRegion grass;
        grass.width = 32;
        grass.height = 32;
        BOOST_REQUIRE(terrain.place("grass", grass));
        BOOST_REQUIRE(first.add(terrain));
        BOOST_REQUIRE(first.add(v3d::asset::media::SpriteSheet({.name = "units", .image = "old.png", .width = 16, .height = 16})));
        BOOST_REQUIRE(v3d::asset::writeDocument(document, first.document()));
    }

    const v3d::image::Packing packing = v3d::image::pack({sprite("archer", 8, 8)}, 256, logger());
    BOOST_REQUIRE(packing.sheet);
    v3d::asset::media::SpriteSheets sheets(logger());
    BOOST_REQUIRE(sheets.read(document));
    BOOST_REQUIRE(sheets.add(v3d::asset::media::SpriteSheet::packed("units", "units.png", packing)));

    BOOST_TEST(sheets.names() == std::vector<std::string>({"terrain", "units"}), boost::test_tools::per_element());
    BOOST_TEST(sheets.get("terrain").has("grass"));
    BOOST_TEST(sheets.get("units").image() == "units.png");
    BOOST_TEST(sheets.get("units").has("archer"));
}

/**
 * A file that is there and cannot be read whole is refused, so writing it back cannot lose part
 * of it. The three are a file that is not JSON, one that is not an object, and one holding a
 * sheet with no image.
 **/
BOOST_AUTO_TEST_CASE(sprite_sheets_read_refuses_a_file_it_cannot_keep_test) {
    const Sandbox sandbox("refuse");
    const char* const documents[] = {
        "{\"sheets\": [",
        "[1, 2, 3]",
        "{\"sheets\": [{\"name\": \"units\", \"width\": 16, \"height\": 16, \"sprites\": []}]}",
    };
    for (const char* const text : documents) {
        BOOST_TEST_CONTEXT(text) {
            const boost::filesystem::path document = sandbox.file("sprites.json");
            BOOST_REQUIRE(v3d::asset::writeFile(document, text));
            v3d::asset::media::SpriteSheets sheets(logger());
            BOOST_TEST(!sheets.read(document));
        }
    }
}

};  // namespace

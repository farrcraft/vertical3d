/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>

#include <boost/test/unit_test.hpp>

#include "../SpriteSheets.h"

#include <boost/make_shared.hpp>

namespace {

boost::shared_ptr<v3d::asset::Json> config(const std::string& text) {
    boost::json::value parsed = boost::json::parse(text);
    return boost::make_shared<v3d::asset::Json>("sprites", v3d::asset::Type::JsonDocument, parsed.as_object());
}

boost::shared_ptr<v3d::log::Logger> logger() {
    return boost::make_shared<v3d::log::Logger>();
}

/**
 * Two sheets, one of them the shape a tile set is: a square image cut into a row of tiles.
 **/
const char* const sheets =
"{\"sheets\": ["
"{\"name\": \"terrain\", \"image\": \"terrain.png\", \"width\": 256, \"height\": 128,"
" \"sprites\": ["
"  {\"name\": \"grass\", \"x\": 0, \"y\": 0, \"width\": 64, \"height\": 64},"
"  {\"name\": \"water\", \"x\": 64, \"y\": 0, \"width\": 64, \"height\": 64},"
"  {\"name\": \"cabin\", \"x\": 128, \"y\": 0, \"width\": 128, \"height\": 128}"
" ]},"
"{\"name\": \"actors\", \"image\": \"actors.png\", \"width\": 64, \"height\": 64,"
" \"sprites\": [{\"name\": \"player\", \"x\": 0, \"y\": 0, \"width\": 32, \"height\": 64}]}"
"]}";

};  // namespace

BOOST_AUTO_TEST_CASE(sprite_sheets_load_test) {
    v3d::config::SpriteSheets loaded(logger());
    BOOST_REQUIRE_EQUAL(loaded.load(config(sheets)), true);

    BOOST_REQUIRE_EQUAL(loaded.names().size(), 2u);
    BOOST_CHECK_EQUAL(loaded.names()[0], "terrain");
    BOOST_CHECK_EQUAL(loaded.names()[1], "actors");
    BOOST_CHECK(loaded.has("terrain"));
    BOOST_CHECK(!loaded.has("nothing"));

    const v3d::config::SpriteSheet terrain = loaded.get("terrain");
    BOOST_CHECK_EQUAL(terrain.name(), "terrain");
    // the image is named rather than loaded, per ADR-0020
    BOOST_CHECK_EQUAL(terrain.image(), "terrain.png");
    BOOST_CHECK_EQUAL(terrain.width(), 256);
    BOOST_CHECK_EQUAL(terrain.height(), 128);
    BOOST_REQUIRE_EQUAL(terrain.sprites().size(), 3u);
    BOOST_CHECK_EQUAL(terrain.sprites()[0], "grass");
}

/**
 * The document holds pixels and the call gives back a fraction, so an author reads the sheet
 * in the units the image is in and a shader gets what it wants.
 **/
BOOST_AUTO_TEST_CASE(sprite_sheets_convert_pixels_to_uv_test) {
    v3d::config::SpriteSheets loaded(logger());
    BOOST_REQUIRE_EQUAL(loaded.load(config(sheets)), true);
    const v3d::config::SpriteSheet terrain = loaded.get("terrain");

    const v3d::config::SpriteRegion water = terrain.get("water");
    BOOST_CHECK_EQUAL(water.x, 64);
    BOOST_CHECK_EQUAL(water.width, 64);

    glm::vec2 uv0(0.0f, 0.0f);
    glm::vec2 uv1(0.0f, 0.0f);
    BOOST_REQUIRE(terrain.uv("water", &uv0, &uv1));
    BOOST_CHECK_CLOSE(uv0.x, 0.25f, 0.001f);
    BOOST_CHECK_CLOSE(uv0.y, 0.0f, 0.001f);
    BOOST_CHECK_CLOSE(uv1.x, 0.5f, 0.001f);
    BOOST_CHECK_CLOSE(uv1.y, 0.5f, 0.001f);

    // the sheet that fills its image is 0..1 in both axes
    BOOST_REQUIRE(terrain.uv("cabin", &uv0, &uv1));
    BOOST_CHECK_CLOSE(uv1.x, 1.0f, 0.001f);
    BOOST_CHECK_CLOSE(uv1.y, 1.0f, 0.001f);

    // a sprite nobody put in the sheet leaves both outputs alone
    glm::vec2 untouched(-1.0f, -1.0f);
    BOOST_CHECK(!terrain.uv("nothing", &untouched, &untouched));
    BOOST_CHECK_CLOSE(untouched.x, -1.0f, 0.001f);
}

/**
 * A sheet with no name, no image or no size is not one, and a region running off the sheet
 * would give a uv outside 0..1 - which samples whatever the wrap mode decides rather than
 * reporting anything. Both are refused, and the rest of the document is kept: one bad entry
 * should not cost an app every sprite it has.
 **/
BOOST_AUTO_TEST_CASE(sprite_sheets_reject_what_they_cannot_use_test) {
    const char* const mixed =
    "{\"sheets\": ["
    "{\"image\": \"unnamed.png\", \"width\": 32, \"height\": 32},"
    "{\"name\": \"sizeless\", \"image\": \"sizeless.png\"},"
    "{\"name\": \"good\", \"image\": \"good.png\", \"width\": 64, \"height\": 64,"
    " \"sprites\": ["
    "  {\"name\": \"fits\", \"x\": 0, \"y\": 0, \"width\": 32, \"height\": 32},"
    "  {\"name\": \"overruns\", \"x\": 48, \"y\": 0, \"width\": 32, \"height\": 32},"
    "  {\"name\": \"negative\", \"x\": -8, \"y\": 0, \"width\": 8, \"height\": 8},"
    "  {\"name\": \"empty\", \"x\": 0, \"y\": 0, \"width\": 0, \"height\": 8}"
    " ]}"
    "]}";

    v3d::config::SpriteSheets loaded(logger());
    BOOST_CHECK_EQUAL(loaded.load(config(mixed)), false);

    BOOST_REQUIRE_EQUAL(loaded.names().size(), 1u);
    BOOST_CHECK_EQUAL(loaded.names()[0], "good");

    const v3d::config::SpriteSheet good = loaded.get("good");
    BOOST_REQUIRE_EQUAL(good.sprites().size(), 1u);
    BOOST_CHECK_EQUAL(good.sprites()[0], "fits");
    BOOST_CHECK(good.has("fits"));
    BOOST_CHECK(!good.has("overruns"));
}

/**
 * A document with no sheets array is not a sprite document, and a sheet nobody loaded hands
 * back an empty one rather than nothing at all.
 **/
BOOST_AUTO_TEST_CASE(sprite_sheets_missing_document_test) {
    v3d::config::SpriteSheets loaded(logger());
    BOOST_CHECK_EQUAL(loaded.load(config("{\"something\": 1}")), false);
    BOOST_CHECK_EQUAL(loaded.load(boost::shared_ptr<v3d::asset::Json>()), false);
    BOOST_CHECK_EQUAL(loaded.names().size(), 0u);

    const v3d::config::SpriteSheet absent = loaded.get("nothing");
    BOOST_CHECK_EQUAL(absent.name(), "");
    BOOST_CHECK_EQUAL(absent.width(), 0);
    BOOST_CHECK_EQUAL(absent.sprites().size(), 0u);

    // a sheet with no size to divide by cannot answer a uv
    glm::vec2 uv0(0.0f, 0.0f);
    glm::vec2 uv1(0.0f, 0.0f);
    BOOST_CHECK(!absent.uv("anything", &uv0, &uv1));
}

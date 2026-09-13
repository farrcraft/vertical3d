/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/config/SpriteSheets.h>
#include <api/asset/Writer.h>

#include <string>

#include <boost/test/unit_test.hpp>

#include <boost/make_shared.hpp>

namespace {

boost::shared_ptr<v3d::asset::kind::Json> config(const std::string& text) {
    boost::json::value parsed = boost::json::parse(text);
    return boost::make_shared<v3d::asset::kind::Json>("sprites", v3d::asset::Type::JsonDocument, parsed.as_object());
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
    BOOST_CHECK_EQUAL(loaded.load(boost::shared_ptr<v3d::asset::kind::Json>()), false);
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

/**
 * The document write() emits is the document load() reads, through the text form that
 * actually reaches a file.
 *
 * This is the case the write side exists for. A packer that emitted the format from its own
 * code would be a second implementation of it, and the two would drift the way this format
 * specialises in: place() drops a region it does not like and keeps the sheet, get() answers
 * a missing name with an empty region, and uv() answers false - so a sheet that stopped
 * being emitted correctly draws as nothing and reports nothing.
 **/
BOOST_AUTO_TEST_CASE(sprite_sheets_round_trip_test) {
    v3d::config::SpriteSheets loaded(logger());
    BOOST_REQUIRE_EQUAL(loaded.load(config(sheets)), true);

    // through serializeDocument rather than straight back, because the text form is what a
    // file holds and a number that widened on the way out would only show up here
    const std::string text = v3d::asset::serializeDocument(loaded.document());
    v3d::config::SpriteSheets reloaded(logger());
    BOOST_REQUIRE_EQUAL(reloaded.load(config(text)), true);

    BOOST_REQUIRE_EQUAL(reloaded.names().size(), loaded.names().size());
    for (size_t i = 0; i < loaded.names().size(); ++i) {
        // the order sheets were written in is the order they come back in, so re-packing
        // one sheet does not move the others in the diff
        BOOST_REQUIRE_EQUAL(reloaded.names()[i], loaded.names()[i]);

        const v3d::config::SpriteSheet before = loaded.get(loaded.names()[i]);
        const v3d::config::SpriteSheet after = reloaded.get(reloaded.names()[i]);
        BOOST_CHECK_EQUAL(after.image(), before.image());
        BOOST_CHECK_EQUAL(after.width(), before.width());
        BOOST_CHECK_EQUAL(after.height(), before.height());

        BOOST_REQUIRE_EQUAL(after.sprites().size(), before.sprites().size());
        for (size_t j = 0; j < before.sprites().size(); ++j) {
            BOOST_REQUIRE_EQUAL(after.sprites()[j], before.sprites()[j]);
            const v3d::config::SpriteRegion one = before.get(before.sprites()[j]);
            const v3d::config::SpriteRegion two = after.get(after.sprites()[j]);
            BOOST_CHECK_EQUAL(two.x, one.x);
            BOOST_CHECK_EQUAL(two.y, one.y);
            BOOST_CHECK_EQUAL(two.width, one.width);
            BOOST_CHECK_EQUAL(two.height, one.height);
        }
    }
}

/**
 * A sheet built to be written refuses exactly what a sheet read from a document refuses, so
 * a packer cannot emit a region the reader would drop on the way back in.
 **/
BOOST_AUTO_TEST_CASE(sprite_sheets_build_a_document_test) {
    v3d::config::SpriteSheet packed("hud", "art/hud.png", 128, 64);
    BOOST_CHECK(packed.place("heart", v3d::config::SpriteRegion()) == false);

    v3d::config::SpriteRegion heart;
    heart.x = 0;
    heart.y = 0;
    heart.width = 16;
    heart.height = 16;
    BOOST_CHECK(packed.place("heart", heart));

    // the same validation place() applies to a region read out of a document
    v3d::config::SpriteRegion overruns;
    overruns.x = 120;
    overruns.y = 0;
    overruns.width = 16;
    overruns.height = 16;
    BOOST_CHECK(!packed.place("overruns", overruns));

    v3d::config::SpriteSheets built(logger());
    BOOST_REQUIRE(built.add(packed));

    v3d::config::SpriteSheets reloaded(logger());
    BOOST_REQUIRE_EQUAL(reloaded.load(config(v3d::asset::serializeDocument(built.document()))), true);
    BOOST_REQUIRE_EQUAL(reloaded.names().size(), 1u);

    const v3d::config::SpriteSheet hud = reloaded.get("hud");
    BOOST_CHECK_EQUAL(hud.image(), "art/hud.png");
    BOOST_REQUIRE_EQUAL(hud.sprites().size(), 1u);
    BOOST_CHECK_EQUAL(hud.sprites()[0], "heart");
    BOOST_CHECK_EQUAL(hud.get("heart").width, 16);
}

/**
 * Packing one sheet of several replaces that sheet and leaves the rest alone, which is the
 * whole of what a tool needs to keep a document it only partly owns.
 **/
BOOST_AUTO_TEST_CASE(sprite_sheets_add_replaces_one_sheet_test) {
    v3d::config::SpriteSheets loaded(logger());
    BOOST_REQUIRE_EQUAL(loaded.load(config(sheets)), true);
    BOOST_REQUIRE_EQUAL(loaded.names().size(), 2u);

    v3d::config::SpriteSheet repacked("terrain", "terrain.png", 512, 512);
    v3d::config::SpriteRegion grass;
    grass.x = 0;
    grass.y = 0;
    grass.width = 128;
    grass.height = 128;
    BOOST_REQUIRE(repacked.place("grass", grass));
    BOOST_REQUIRE(loaded.add(repacked));

    // replaced rather than merged into, and it kept its place in the order
    BOOST_REQUIRE_EQUAL(loaded.names().size(), 2u);
    BOOST_CHECK_EQUAL(loaded.names()[0], "terrain");
    const v3d::config::SpriteSheet terrain = loaded.get("terrain");
    BOOST_CHECK_EQUAL(terrain.width(), 512);
    BOOST_REQUIRE_EQUAL(terrain.sprites().size(), 1u);
    BOOST_CHECK(!terrain.has("water"));

    // and the sheet nobody packed is untouched, which is what loading before writing buys
    const v3d::config::SpriteSheet actors = loaded.get("actors");
    BOOST_CHECK_EQUAL(actors.image(), "actors.png");
    BOOST_CHECK(actors.has("player"));

    // a sheet that could not be read back is not one that can be put in
    BOOST_CHECK(!loaded.add(v3d::config::SpriteSheet()));
    BOOST_CHECK(!loaded.add(v3d::config::SpriteSheet("sizeless", "sizeless.png", 0, 0)));
    BOOST_CHECK_EQUAL(loaded.names().size(), 2u);
}

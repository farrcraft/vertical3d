/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/Type.h>
#include <api/asset/Worker.h>
#include <api/asset/kind/Json.h>
#include <api/grid/TileCoord.h>
#include <api/grid/TileGrid.h>
#include <api/log/Logger.h>
#include <odyssey/tile/World.h>

#include <chrono>
#include <cstddef>
#include <fstream>
#include <optional>
#include <string>
#include <thread>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec3.hpp>

using odyssey::tile::World;
using v3d::grid::TileCoord;

namespace {

/**
 * Two 6 x 4 rooms side by side, with a doorway through the wall between them on row 1. The
 * west room is region 0 and holds the start.
 **/
const char* const west =
"{\"tiles\": ["
"\"######\","
"\"#@....\","
"\"#....#\","
"\"######\""
"]}";

const char* const east =
"{\"tiles\": ["
"\"######\","
"\".....#\","
"\"#....#\","
"\"######\""
"]}";

const char* const twoRegions =
"{\"regions\": ["
"{\"file\": \"west.json\", \"corner\": {\"x\": 0, \"y\": 0}, \"size\": {\"x\": 6, \"y\": 4}},"
"{\"file\": \"east.json\", \"corner\": {\"x\": 6, \"y\": 0}, \"size\": {\"x\": 6, \"y\": 4}}"
"], \"start\": 0}";

/**
 * A directory holding the region files, and a world reading from it through a worker.
 **/
struct Fixture final {
    explicit Fixture(const std::string& name, bool withEast = true) :
        logger(boost::make_shared<v3d::log::Logger>()),
        worker(boost::make_shared<v3d::asset::Worker>()) {
        const boost::filesystem::path directory = boost::filesystem::path("data_out") / name;
        boost::filesystem::remove_all(directory);
        boost::filesystem::create_directories(directory);
        std::ofstream((directory / "west.json").string()) << west;
        if (withEast) {
            std::ofstream((directory / "east.json").string()) << east;
        }
        assets = boost::make_shared<v3d::asset::Manager>(directory.string(), logger);
        world = boost::make_shared<World>(logger, assets, worker);
    }

    /**
     * Poll the worker until it has nothing left, or ten seconds pass.
     **/
    bool drain() {
        const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (worker->outstanding() > 0) {
            if (std::chrono::steady_clock::now() > deadline) {
                return false;
            }
            worker->poll();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return true;
    }

    boost::shared_ptr<v3d::log::Logger> logger;
    boost::shared_ptr<v3d::asset::Worker> worker;
    boost::shared_ptr<v3d::asset::Manager> assets;
    boost::shared_ptr<World> world;
};

boost::shared_ptr<v3d::asset::kind::Json> document(const std::string& text) {
    boost::json::value parsed = boost::json::parse(text);
    return boost::make_shared<v3d::asset::kind::Json>("world", v3d::asset::Type::JsonDocument, parsed.as_object());
}

// the preprocessor splits a macro argument on every comma outside parentheses, so a braced
// TileCoord cannot be written inside a BOOST_CHECK
constexpr TileCoord at(int x, int y) {
    return TileCoord{ x, y };
}

BOOST_AUTO_TEST_SUITE(world_test)

/**
 * The start region is loaded at once and the others are not. The start is a world tile, and a
 * tile in a region not loaded cannot be entered.
 **/
BOOST_AUTO_TEST_CASE(the_start_region_loads_at_once) {
    const Fixture fixture("world_start");
    BOOST_REQUIRE(fixture.world->load(document(twoRegions)));

    BOOST_CHECK_EQUAL(fixture.world->regions(), 2U);
    BOOST_CHECK(fixture.world->loaded(0));
    BOOST_CHECK(!fixture.world->loaded(1));
    BOOST_CHECK(fixture.world->start() == at(1, 1));
    BOOST_CHECK(fixture.world->passable(at(5, 1)));
    BOOST_CHECK(!fixture.world->passable(at(6, 1)));
    BOOST_CHECK(fixture.world->regionAt(at(6, 1)) == std::optional<std::size_t>(1));
    BOOST_CHECK(!fixture.world->regionAt(at(12, 1)));
    BOOST_CHECK(fixture.world->min() == at(0, 0));
    BOOST_CHECK(fixture.world->max() == at(12, 4));
}

/**
 * A region approached is loaded on the worker, and its grid is placed by its corner, so the
 * grid's own conversion and the world's agree on which of its tiles a world tile is.
 **/
BOOST_AUTO_TEST_CASE(an_approached_region_loads_placed_by_its_corner) {
    Fixture fixture("world_approach");
    BOOST_REQUIRE(fixture.world->load(document(twoRegions)));

    // four tiles from the east region is APPROACH, and five is not
    fixture.world->stream(at(1, 1));
    BOOST_CHECK(!fixture.world->loading(1));
    fixture.world->stream(at(2, 1));
    BOOST_CHECK(fixture.world->loading(1));
    BOOST_REQUIRE(fixture.drain());
    BOOST_REQUIRE(fixture.world->loaded(1));
    BOOST_CHECK(!fixture.world->loading(1));

    const v3d::grid::TileGrid& grid = *fixture.world->map(1)->grid();
    BOOST_CHECK(grid.worldMin() == glm::vec3(6.0F, 0.0F, 0.0F));
    for (int y = 0; y < 4; y++) {
        for (int x = 6; x < 12; x++) {
            const glm::vec3 centre(static_cast<float>(x) + 0.5F, 0.0F, static_cast<float>(y) + 0.5F);
            BOOST_CHECK(grid.worldToTile(centre) == fixture.world->local(1, at(x, y)));
            BOOST_CHECK(fixture.world->world(1, fixture.world->local(1, at(x, y))) == at(x, y));
        }
    }
    // the doorway crosses the seam once both sides are loaded
    BOOST_CHECK(fixture.world->passable(at(6, 1)));
}

/**
 * Crossing into the next region puts the one left behind out of sight. What was seen there stays
 * remembered, and is no longer current.
 **/
BOOST_AUTO_TEST_CASE(crossing_a_seam_puts_the_old_region_out_of_sight) {
    Fixture fixture("world_cross");
    BOOST_REQUIRE(fixture.world->load(document(twoRegions)));
    fixture.world->stream(at(5, 1));
    BOOST_REQUIRE(fixture.drain());

    fixture.world->look(at(5, 1));
    BOOST_CHECK(fixture.world->sight(0).visible(at(4, 1)));

    fixture.world->look(at(6, 1));
    BOOST_CHECK(!fixture.world->sight(0).visible(at(4, 1)));
    BOOST_CHECK(fixture.world->sight(0).remembered(at(4, 1)));
    BOOST_CHECK(fixture.world->sight(1).visible(at(0, 1)));
}

/**
 * A world loaded over another while a region of the old one is on the worker is not filled by
 * that region when it arrives.
 **/
BOOST_AUTO_TEST_CASE(a_world_loaded_over_another_ignores_the_old_loads) {
    Fixture fixture("world_reload");
    BOOST_REQUIRE(fixture.world->load(document(twoRegions)));
    fixture.world->stream(at(2, 1));
    BOOST_REQUIRE(fixture.world->loading(1));

    BOOST_REQUIRE(fixture.world->load(document(twoRegions)));
    BOOST_REQUIRE(fixture.drain());
    BOOST_CHECK(!fixture.world->loaded(1));
    BOOST_CHECK(!fixture.world->loading(1));
}

/**
 * A region the player is far from is not asked for, and one it leaves behind is released. What
 * was seen of it is remembered when it is loaded again.
 **/
BOOST_AUTO_TEST_CASE(a_region_left_behind_is_released_and_remembered) {
    Fixture fixture("world_release");
    const std::string wide =
        "{\"regions\": ["
        "{\"file\": \"west.json\", \"corner\": {\"x\": 0, \"y\": 0}, \"size\": {\"x\": 6, \"y\": 4}},"
        "{\"file\": \"east.json\", \"corner\": {\"x\": 30, \"y\": 0}, \"size\": {\"x\": 6, \"y\": 4}}"
        "], \"start\": 0}";
    BOOST_REQUIRE(fixture.world->load(document(wide)));

    // 25 tiles from the east region, beyond APPROACH
    fixture.world->stream(at(5, 1));
    BOOST_CHECK(!fixture.world->loading(1));

    // standing in the east region loads it, and a look remembers a tile of it
    fixture.world->stream(at(30, 1));
    BOOST_REQUIRE(fixture.drain());
    BOOST_REQUIRE(fixture.world->loaded(1));
    fixture.world->look(at(31, 1));
    BOOST_CHECK(fixture.world->sight(1).remembered(at(1, 1)));

    // the west region was left behind in turn while the player stood in the east one
    BOOST_CHECK(!fixture.world->loaded(0));

    // back west, more than DEPART tiles from the east region, which is released while the west
    // one is asked for again
    fixture.world->stream(at(1, 1));
    BOOST_CHECK(!fixture.world->loaded(1));
    BOOST_CHECK(fixture.world->sight(1).remembered(at(1, 1)));
    BOOST_CHECK(fixture.world->loading(0));
    BOOST_REQUIRE(fixture.drain());
    BOOST_CHECK(fixture.world->loaded(0));
}

/**
 * A region released while its load is on the worker drops the load when it arrives.
 **/
BOOST_AUTO_TEST_CASE(a_load_for_a_released_region_is_dropped) {
    Fixture fixture("world_dropped");
    const std::string wide =
        "{\"regions\": ["
        "{\"file\": \"west.json\", \"corner\": {\"x\": 0, \"y\": 0}, \"size\": {\"x\": 6, \"y\": 4}},"
        "{\"file\": \"east.json\", \"corner\": {\"x\": 20, \"y\": 0}, \"size\": {\"x\": 6, \"y\": 4}}"
        "], \"start\": 0}";
    BOOST_REQUIRE(fixture.world->load(document(wide)));

    fixture.world->stream(at(17, 1));
    BOOST_CHECK(fixture.world->loading(1));
    fixture.world->stream(at(1, 1));
    BOOST_CHECK(!fixture.world->loading(1));

    BOOST_REQUIRE(fixture.drain());
    BOOST_CHECK(!fixture.world->loaded(1));
}

/**
 * A region whose map cannot be read is reported once, and asked for again only after the player
 * has gone away from it and come back.
 **/
BOOST_AUTO_TEST_CASE(a_region_that_cannot_be_read_is_not_asked_for_every_frame) {
    Fixture fixture("world_failed", false);
    const std::string wide =
        "{\"regions\": ["
        "{\"file\": \"west.json\", \"corner\": {\"x\": 0, \"y\": 0}, \"size\": {\"x\": 6, \"y\": 4}},"
        "{\"file\": \"east.json\", \"corner\": {\"x\": 20, \"y\": 0}, \"size\": {\"x\": 6, \"y\": 4}}"
        "], \"start\": 0}";
    BOOST_REQUIRE(fixture.world->load(document(wide)));

    fixture.world->stream(at(17, 1));
    BOOST_REQUIRE(fixture.drain());
    BOOST_CHECK(!fixture.world->loaded(1));
    fixture.world->stream(at(17, 1));
    BOOST_CHECK(!fixture.world->loading(1));

    fixture.world->stream(at(1, 1));
    fixture.world->stream(at(17, 1));
    BOOST_CHECK(fixture.world->loading(1));
    BOOST_REQUIRE(fixture.drain());
}

/**
 * A world destroyed while a region is on the worker leaves its completion nothing to fill.
 **/
BOOST_AUTO_TEST_CASE(a_world_destroyed_before_its_loads_arrive) {
    Fixture fixture("world_destroyed");
    BOOST_REQUIRE(fixture.world->load(document(twoRegions)));

    fixture.world->stream(at(1, 1));
    fixture.world.reset();
    BOOST_CHECK(fixture.drain());
}

/**
 * The view keeps the player as near the middle of the screen as the world's edges allow.
 **/
BOOST_AUTO_TEST_CASE(the_view_follows_the_player_inside_the_world) {
    const Fixture fixture("world_view");
    BOOST_REQUIRE(fixture.world->load(document(twoRegions)));
    const TileCoord screen = at(4, 2);

    BOOST_CHECK(fixture.world->view(at(1, 1), screen) == at(0, 0));
    BOOST_CHECK(fixture.world->view(at(6, 2), screen) == at(4, 1));
    BOOST_CHECK(fixture.world->view(at(11, 3), screen) == at(8, 2));
    // a world smaller than the screen is drawn from its least tile
    BOOST_CHECK(fixture.world->view(at(6, 2), at(40, 40)) == at(0, 0));
}

/**
 * A document with overlapping regions, a region whose map is not the size it says, a corner that
 * is not a whole number, or a start that names no region is refused.
 **/
BOOST_AUTO_TEST_CASE(a_world_that_cannot_be_read_is_refused) {
    const Fixture fixture("world_refused");
    const char* const overlapping =
        "{\"regions\": ["
        "{\"file\": \"west.json\", \"corner\": {\"x\": 0, \"y\": 0}, \"size\": {\"x\": 6, \"y\": 4}},"
        "{\"file\": \"east.json\", \"corner\": {\"x\": 5, \"y\": 0}, \"size\": {\"x\": 6, \"y\": 4}}"
        "], \"start\": 0}";
    const char* const refused[] = {
        overlapping,
        "{\"regions\": [{\"file\": \"west.json\", \"corner\": {\"x\": 0, \"y\": 0}, \"size\": {\"x\": 7, \"y\": 4}}], \"start\": 0}",
        "{\"regions\": [{\"file\": \"west.json\", \"corner\": {\"x\": 0.5, \"y\": 0}, \"size\": {\"x\": 6, \"y\": 4}}], \"start\": 0}",
        "{\"regions\": [{\"file\": \"west.json\", \"corner\": {\"x\": 0, \"y\": 0}, \"size\": {\"x\": 6, \"y\": 4}}], \"start\": 1}",
        "{\"regions\": [], \"start\": 0}",
    };
    for (const char* const text : refused) {
        BOOST_TEST_CONTEXT(text) {
            BOOST_CHECK(!fixture.world->load(document(text)));
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

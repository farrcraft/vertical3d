/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#include <api/grid/Pathfinding.h>
#include <odyssey/tile/Map.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

boost::shared_ptr<v3d::asset::kind::Json> document(const std::string& text) {
    boost::json::value parsed = boost::json::parse(text);
    return boost::make_shared<v3d::asset::kind::Json>("map", v3d::asset::Type::JsonDocument, parsed.as_object());
}

boost::shared_ptr<odyssey::tile::Map> map() {
    return boost::make_shared<odyssey::tile::Map>(boost::make_shared<v3d::log::Logger>());
}

/**
 * A room with a doorway in the middle of its wall, so a route across it has to find the
 * one gap rather than walk the straight line the distance metric would suggest.
 **/
const char* const room =
"{\"tiles\": ["
"\"#####\","
"\"#@#.#\","
"\"#.#.#\","
"\"#...#\","
"\"#####\""
"]}";

};  // namespace

/**
 * The document is read as it is written: the first string is the northmost row, a
 * character is a tile, and the kinds land where the file put them.
 **/
BOOST_AUTO_TEST_CASE(map_load_test) {
    boost::shared_ptr<odyssey::tile::Map> board = map();
    BOOST_REQUIRE(board->load(document(room)));

    BOOST_REQUIRE(board->loaded());
    BOOST_CHECK_EQUAL(board->grid()->width(), 5);
    BOOST_CHECK_EQUAL(board->grid()->height(), 5);

    BOOST_CHECK(board->kind({0, 0}) == odyssey::tile::Kind::Wall);
    BOOST_CHECK(board->kind({1, 1}) == odyssey::tile::Kind::Floor);
    BOOST_CHECK(board->kind({2, 1}) == odyssey::tile::Kind::Wall);

    // '@' is where the player stands, and is floor like any other
    BOOST_CHECK(board->start() == v3d::grid::TileCoord({1, 1}));
}

/**
 * A kind decides passability and cover together, which is the only place those two are
 * chosen - the grid holds them and has no opinion about what a crate is.
 **/
BOOST_AUTO_TEST_CASE(map_kinds_carry_cover_test) {
    boost::shared_ptr<odyssey::tile::Map> board = map();
    BOOST_REQUIRE(board->load(document("{\"tiles\": [\"#.o\"]}")));

    BOOST_CHECK(!board->grid()->passable({0, 0}));
    BOOST_CHECK(board->grid()->cover({0, 0}) == v3d::grid::Cover::Full);

    BOOST_CHECK(board->grid()->passable({1, 0}));
    BOOST_CHECK(board->grid()->cover({1, 0}) == v3d::grid::Cover::None);

    // a crate stops a mover and not a line of sight
    BOOST_CHECK(!board->grid()->passable({2, 0}));
    BOOST_CHECK(board->grid()->cover({2, 0}) == v3d::grid::Cover::Half);
}

/**
 * The whole point of the map: a route over it, found by the api rather than by the app.
 * The straight line from the start to the far corner crosses a wall, so a path that goes
 * around is a path the grid actually searched for.
 **/
BOOST_AUTO_TEST_CASE(map_is_routable_test) {
    boost::shared_ptr<odyssey::tile::Map> board = map();
    BOOST_REQUIRE(board->load(document(room)));

    const v3d::grid::TileCoord goal{3, 1};
    const std::vector<v3d::grid::TileCoord> path =
        v3d::grid::findPath(*board->grid(), board->start(), goal);

    BOOST_REQUIRE(!path.empty());
    BOOST_CHECK(path.front() == board->start());
    BOOST_CHECK(path.back() == goal);
    // three tiles apart on open ground, but the wall between them makes it four steps
    BOOST_CHECK_EQUAL(v3d::grid::tileDistance(board->start(), goal), 2);
    BOOST_CHECK_EQUAL(path.size() - 1, 4u);

    // and every tile of it is one the map says can be walked on
    for (const v3d::grid::TileCoord& tile : path) {
        BOOST_CHECK(board->grid()->passable(tile));
    }
}

/**
 * A map with no '@' still has somewhere to stand, because a board that loaded but cannot
 * be played on is worse than one that did not load.
 **/
BOOST_AUTO_TEST_CASE(map_start_defaults_to_floor_test) {
    boost::shared_ptr<odyssey::tile::Map> board = map();
    BOOST_REQUIRE(board->load(document("{\"tiles\": [\"##\", \"#.\"]}")));

    BOOST_CHECK(board->start() == v3d::grid::TileCoord({1, 1}));
}

/**
 * A document that is not a map is rejected rather than patched, and the map that was
 * loaded before survives the attempt.
 **/
BOOST_AUTO_TEST_CASE(map_rejects_test) {
    boost::shared_ptr<odyssey::tile::Map> board = map();
    BOOST_REQUIRE(board->load(document(room)));

    BOOST_CHECK(!board->load(document("{}")));
    BOOST_CHECK(!board->load(document("{\"tiles\": []}")));
    // rows of different lengths are not a rectangle
    BOOST_CHECK(!board->load(document("{\"tiles\": [\"##\", \"###\"]}")));
    // a character naming no kind
    BOOST_CHECK(!board->load(document("{\"tiles\": [\"#?#\"]}")));
    // nowhere to stand
    BOOST_CHECK(!board->load(document("{\"tiles\": [\"###\"]}")));

    // the room is still the loaded map
    BOOST_CHECK_EQUAL(board->grid()->width(), 5);
    BOOST_CHECK(board->start() == v3d::grid::TileCoord({1, 1}));
}

/**
 * Nothing stands outside the board, so a tile off it reads as wall rather than throwing -
 * the same shape TileGrid::passable() has for the same reason.
 **/
BOOST_AUTO_TEST_CASE(map_off_the_board_test) {
    boost::shared_ptr<odyssey::tile::Map> board = map();
    BOOST_REQUIRE(board->load(document(room)));

    BOOST_CHECK(board->kind({-1, 0}) == odyssey::tile::Kind::Wall);
    BOOST_CHECK(board->kind({5, 5}) == odyssey::tile::Kind::Wall);
}

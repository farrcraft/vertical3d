/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#include <odyssey/tile/Map.h>
#include <odyssey/tile/Sight.h>

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

boost::shared_ptr<v3d::asset::kind::Json> document(const std::string& text) {
    boost::json::value parsed = boost::json::parse(text);
    return boost::make_shared<v3d::asset::kind::Json>("map", v3d::asset::Type::JsonDocument, parsed.as_object());
}

/**
 * A board of one row, so what is between two tiles is what the row says is between them
 * and nothing is reached around.
 *
 * @param glyphs the row's characters, in the map document's own alphabet
 **/
boost::shared_ptr<v3d::grid::TileGrid> row(const std::string& glyphs) {
    boost::shared_ptr<odyssey::tile::Map> board =
        boost::make_shared<odyssey::tile::Map>(boost::make_shared<v3d::log::Logger>());
    BOOST_REQUIRE(board->load(document("{\"tiles\": [\"" + glyphs + "\"]}")));
    // the grid outlives the map it was read from
    return board->grid();
}

constexpr v3d::grid::TileCoord at(int x) {
    return v3d::grid::TileCoord{ x, 0 };
}

};  // namespace

/**
 * A kind's cover is what decides sight, which is the one place this app's alphabet and the
 * grid's rules meet: a crate is Cover::Half and is seen over, a wall is Cover::Full and is
 * not. Both are impassable, so passability says nothing about either.
 **/
BOOST_AUTO_TEST_CASE(sight_reads_cover_test) {
    odyssey::tile::Sight seen;

    seen.look(*row("#@.o..#"), at(1));
    // the crate is in sight itself, and so is the floor behind it
    BOOST_CHECK(seen.visible(at(3)));
    BOOST_CHECK(seen.visible(at(5)));

    seen.look(*row("#@.#..#"), at(1));
    // the wall is in sight, because a line's own endpoints are never tested, and what it
    // stands in front of is not
    BOOST_CHECK(seen.visible(at(3)));
    BOOST_CHECK(!seen.visible(at(4)));
    BOOST_CHECK(!seen.visible(at(5)));
}

/**
 * Sight reaches Sight::RANGE tiles and no further, down a row with nothing in it. The range
 * is this game's rather than the grid's - v3d::grid::hasLineOfSight has no distance at all.
 **/
BOOST_AUTO_TEST_CASE(sight_has_a_range_test) {
    odyssey::tile::Sight seen;
    const boost::shared_ptr<v3d::grid::TileGrid> open = row("#@................#");

    seen.look(*open, at(1));
    BOOST_CHECK(seen.visible(at(1)));
    BOOST_CHECK(seen.visible(at(1 + odyssey::tile::Sight::RANGE)));
    BOOST_CHECK(!seen.visible(at(1 + odyssey::tile::Sight::RANGE + 1)));
}

/**
 * What has been seen stays known after the viewer walks away from it, and a board of
 * another size is another board rather than the same one seen again.
 **/
BOOST_AUTO_TEST_CASE(sight_remembers_test) {
    odyssey::tile::Sight seen;
    const boost::shared_ptr<v3d::grid::TileGrid> open = row("#@................#");

    // nothing is known before the first look
    BOOST_CHECK(!seen.visible(at(2)));
    BOOST_CHECK(!seen.remembered(at(2)));

    seen.look(*open, at(1));
    BOOST_CHECK(seen.remembered(at(2)));

    seen.look(*open, at(17));
    BOOST_CHECK(seen.visible(at(17)));
    BOOST_CHECK(!seen.visible(at(2)));
    BOOST_CHECK(seen.remembered(at(2)));

    // a board of another size is not the same board seen again: (3) is behind a wall here
    // and was in sight on the board before
    seen.look(*row("#@#....#"), at(1));
    BOOST_CHECK(!seen.visible(at(3)));
    BOOST_CHECK(!seen.remembered(at(3)));
}

/**
 * Nothing off the board is seen or remembered, so a caller drawing a tile asks about it
 * rather than about the bounds first - the shape TileGrid::passable() has for the same
 * reason.
 **/
BOOST_AUTO_TEST_CASE(sight_off_the_board_test) {
    odyssey::tile::Sight seen;
    seen.look(*row("#@.....#"), at(1));

    BOOST_CHECK(!seen.visible(at(-1)));
    BOOST_CHECK(!seen.remembered(at(-1)));
    BOOST_CHECK(!seen.visible(at(8)));
    BOOST_CHECK(!seen.remembered(v3d::grid::TileCoord{1, 3}));
}

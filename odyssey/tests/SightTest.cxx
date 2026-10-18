/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2026 Joshua Farr (josh@farrcraft.com)
 **/

#include <api/asset/Type.h>
#include <api/asset/kind/Json.h>
#include <api/grid/TileCoord.h>
#include <api/grid/TileGrid.h>
#include <api/log/Logger.h>
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
 * A board one row tall. Every tile between two tiles is in the row, so no line of sight can go
 * round an obstacle.
 *
 * @param glyphs the row's characters, in the map document's own alphabet
 **/
boost::shared_ptr<v3d::grid::TileGrid> row(const std::string& glyphs) {
    const boost::shared_ptr<odyssey::tile::Map> board =
        boost::make_shared<odyssey::tile::Map>(boost::make_shared<v3d::log::Logger>());
    BOOST_REQUIRE(board->load(document("{\"tiles\": [\"" + glyphs + "\"]}")));
    // the grid outlives the map it was read from
    return board->grid();
}

constexpr v3d::grid::TileCoord at(int x) {
    return v3d::grid::TileCoord{ x, 0 };
}

/**
 * A kind's cover decides sight: a crate is Cover::Half and is seen over, and a wall is
 * Cover::Full and is not. Both are impassable, so passability does not affect sight.
 **/
BOOST_AUTO_TEST_CASE(sight_reads_cover_test) {
    odyssey::tile::Sight seen;

    seen.look(*row("#@.o..#"), at(1));
    // the crate is in sight itself, and so is the floor behind it
    BOOST_CHECK(seen.visible(at(3)));
    BOOST_CHECK(seen.visible(at(5)));

    seen.look(*row("#@.#..#"), at(1));
    // the wall itself is in sight, because hasLineOfSight never tests a line's endpoints. The
    // tiles behind the wall are not
    BOOST_CHECK(seen.visible(at(3)));
    BOOST_CHECK(!seen.visible(at(4)));
    BOOST_CHECK(!seen.visible(at(5)));
}

/**
 * Sight reaches Sight::RANGE tiles and no further, down a row with nothing in it. The range
 * belongs to Sight, because v3d::grid::hasLineOfSight has no distance limit.
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
 * What has been seen stays remembered after the viewer walks away from it. A board of another
 * size starts with nothing remembered.
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

    // tile 3 was seen on the board before. On this board of another size it is behind a wall,
    // and is neither in sight nor remembered
    seen.look(*row("#@#....#"), at(1));
    BOOST_CHECK(!seen.visible(at(3)));
    BOOST_CHECK(!seen.remembered(at(3)));
}

/**
 * A tile off the board is neither in sight nor remembered. A caller can query any tile
 * without checking the bounds first, as with TileGrid::passable().
 **/
BOOST_AUTO_TEST_CASE(sight_off_the_board_test) {
    odyssey::tile::Sight seen;
    seen.look(*row("#@.....#"), at(1));

    BOOST_CHECK(!seen.visible(at(-1)));
    BOOST_CHECK(!seen.remembered(at(-1)));
    BOOST_CHECK(!seen.visible(at(8)));
    BOOST_CHECK(!seen.remembered(v3d::grid::TileCoord{1, 3}));
}

};  // namespace

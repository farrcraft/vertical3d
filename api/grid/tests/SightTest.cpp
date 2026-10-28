/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/grid/LineOfSight.h>
#include <api/grid/Sight.h>
#include <api/grid/TileCoord.h>
#include <api/grid/TileGrid.h>

#include <cstddef>
#include <string>

#include <boost/test/unit_test.hpp>

using v3d::grid::Cover;
using v3d::grid::Sight;
using v3d::grid::TileCoord;
using v3d::grid::TileGrid;

namespace {

/**
 * A board one row tall. Every tile between two tiles is in the row, so no line of sight can go
 * round an obstacle.
 *
 * @param glyphs one character a tile: '#' is a wall, Cover::Full, 'o' is a crate, Cover::Half,
 *        and anything else is open floor
 **/
TileGrid row(const std::string& glyphs) {
    TileGrid grid(static_cast<int>(glyphs.size()), 1);
    for (int x = 0; x < grid.width(); ++x) {
        const char glyph = glyphs[static_cast<std::size_t>(x)];
        if (glyph == '#' || glyph == 'o') {
            grid.setPassable(TileCoord{x, 0}, false);
            grid.setCover(TileCoord{x, 0}, glyph == '#' ? Cover::Full : Cover::Half);
        }
    }
    return grid;
}

constexpr TileCoord at(int x) {
    return TileCoord{ x, 0 };
}

/**
 * Cover decides sight: a crate is Cover::Half and is seen over, and a wall is Cover::Full and
 * is not. Both are impassable, so passability does not affect sight.
 **/
BOOST_AUTO_TEST_CASE(sight_reads_cover_test) {
    Sight seen(7);

    seen.look(row("#..o..#"), at(1));
    // the crate is in sight itself, and so is the floor behind it
    BOOST_CHECK(seen.visible(at(3)));
    BOOST_CHECK(seen.visible(at(5)));

    seen.look(row("#..#..#"), at(1));
    // the wall itself is in sight, because hasLineOfSight never tests a line's endpoints. The
    // tiles behind the wall are not
    BOOST_CHECK(seen.visible(at(3)));
    BOOST_CHECK(!seen.visible(at(4)));
    BOOST_CHECK(!seen.visible(at(5)));
}

/**
 * Sight reaches the range it was given and no further, down a row with nothing in it. The
 * range belongs to Sight, because hasLineOfSight() has no distance limit. A range below zero
 * sees nothing.
 **/
BOOST_AUTO_TEST_CASE(sight_has_the_range_it_was_given_test) {
    const TileGrid open = row("#.................#");
    for (const int range : { 3, 7 }) {
        BOOST_TEST_CONTEXT("range " << range) {
            Sight seen(range);
            BOOST_CHECK_EQUAL(seen.range(), range);
            seen.look(open, at(1));
            BOOST_CHECK(seen.visible(at(1)));
            BOOST_CHECK(seen.visible(at(1 + range)));
            BOOST_CHECK(!seen.visible(at(1 + range + 1)));
        }
    }

    Sight none(-1);
    none.look(open, at(1));
    BOOST_CHECK(!none.visible(at(1)));
}

/**
 * What has been seen stays remembered after the viewer walks away from it. A board of another
 * size starts with nothing remembered.
 **/
BOOST_AUTO_TEST_CASE(sight_remembers_test) {
    Sight seen(7);
    const TileGrid open = row("#.................#");

    // nothing is known before the first look
    BOOST_CHECK(!seen.visible(at(2)));
    BOOST_CHECK(!seen.remembered(at(2)));

    seen.look(open, at(1));
    BOOST_CHECK(seen.remembered(at(2)));

    seen.look(open, at(17));
    BOOST_CHECK(seen.visible(at(17)));
    BOOST_CHECK(!seen.visible(at(2)));
    BOOST_CHECK(seen.remembered(at(2)));

    // tile 3 was seen on the board before. On this board of another size it is behind a wall,
    // and is neither in sight nor remembered
    seen.look(row("#.#....#"), at(1));
    BOOST_CHECK(!seen.visible(at(3)));
    BOOST_CHECK(!seen.remembered(at(3)));

    // blind puts everything out of sight and forgets nothing
    seen.blind();
    BOOST_CHECK(!seen.visible(at(1)));
    BOOST_CHECK(seen.remembered(at(1)));
}

/**
 * A blocker stops sight as a wall does, on top of the grid's own cover, and the tile it blocks
 * is still in sight itself.
 **/
BOOST_AUTO_TEST_CASE(sight_takes_a_blocker_test) {
    Sight seen(7);
    seen.look(row("#......#"), at(1), [](TileCoord tile) { return tile.x == 3; });

    BOOST_CHECK(seen.visible(at(3)));
    BOOST_CHECK(!seen.visible(at(4)));
}

/**
 * A tile off the board is neither in sight nor remembered. A caller can query any tile
 * without checking the bounds first, as with TileGrid::passable().
 **/
BOOST_AUTO_TEST_CASE(sight_off_the_board_test) {
    Sight seen(7);
    seen.look(row("#......#"), at(1));

    BOOST_CHECK(!seen.visible(at(-1)));
    BOOST_CHECK(!seen.remembered(at(-1)));
    BOOST_CHECK(!seen.visible(at(8)));
    BOOST_CHECK(!seen.remembered(TileCoord{1, 3}));
}

};  // namespace

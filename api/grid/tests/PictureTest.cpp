/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/grid/Picture.h>

#include <map>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

using v3d::grid::Cover;
using v3d::grid::Terrain;
using v3d::grid::TileCoord;

namespace {

/**
 * Floor, wall and a low crate - the three kinds of terrain both formats in the tree describe.
 **/
std::map<char, Terrain> legend() {
    return {
        {'.', Terrain{true, Cover::None}},
        {'#', Terrain{false, Cover::Full}},
        {'o', Terrain{false, Cover::Half}}
    };
}

};  // namespace

BOOST_AUTO_TEST_SUITE(picture_test)

/**
 * Every tile takes the terrain its glyph names, with picture row y as tile row y.
 **/
BOOST_AUTO_TEST_CASE(terrain_matches_the_legend_tile_for_tile) {
    const v3d::grid::Picture picture = v3d::grid::fromPicture({"#.o", "..#"}, legend());

    if (!picture.grid) {
        BOOST_ERROR("the picture was refused: " << picture.error);
        return;
    }
    const v3d::grid::TileGrid& grid = *picture.grid;
    BOOST_CHECK(picture.unknown.empty());
    BOOST_CHECK_EQUAL(grid.width(), 3);
    BOOST_CHECK_EQUAL(grid.height(), 2);

    BOOST_CHECK(!grid.passable({0, 0}));
    BOOST_CHECK(grid.cover({0, 0}) == Cover::Full);
    BOOST_CHECK(grid.passable({1, 0}));
    BOOST_CHECK(grid.cover({1, 0}) == Cover::None);
    BOOST_CHECK(!grid.passable({2, 0}));
    BOOST_CHECK(grid.cover({2, 0}) == Cover::Half);
    BOOST_CHECK(grid.passable({0, 1}));
    BOOST_CHECK(!grid.passable({2, 1}));
}

/**
 * A glyph the legend does not name is handed back at every tile it stands on, and those
 * tiles are left impassable with no cover for the caller to decide.
 **/
BOOST_AUTO_TEST_CASE(an_unknown_glyph_is_handed_back) {
    const v3d::grid::Picture picture = v3d::grid::fromPicture({"@.", ".@"}, legend());

    if (!picture.grid) {
        BOOST_ERROR("the picture was refused: " << picture.error);
        return;
    }
    const v3d::grid::TileGrid& grid = *picture.grid;
    BOOST_REQUIRE_EQUAL(picture.unknown.size(), 1u);
    BOOST_CHECK_EQUAL(picture.unknown[0].glyph, '@');
    const std::vector<TileCoord>& at = picture.unknown[0].tiles;
    BOOST_REQUIRE_EQUAL(at.size(), 2u);
    BOOST_CHECK(at[0] == TileCoord({0, 0}));
    BOOST_CHECK(at[1] == TileCoord({1, 1}));
    BOOST_CHECK(!grid.passable({0, 0}));
    BOOST_CHECK(grid.cover({0, 0}) == Cover::None);
}

/**
 * Rows of differing lengths and an empty picture have no tile to hand back, so they are
 * refused, with a reason.
 **/
BOOST_AUTO_TEST_CASE(a_picture_that_is_not_a_rectangle_is_refused) {
    const v3d::grid::Picture ragged = v3d::grid::fromPicture({"##", "###"}, legend());
    BOOST_CHECK(!ragged.grid.has_value());
    BOOST_CHECK(!ragged.error.empty());

    const v3d::grid::Picture empty = v3d::grid::fromPicture({}, legend());
    BOOST_CHECK(!empty.grid.has_value());
    BOOST_CHECK(!empty.error.empty());

    const v3d::grid::Picture blank = v3d::grid::fromPicture({""}, legend());
    BOOST_CHECK(!blank.grid.has_value());
    BOOST_CHECK(!blank.error.empty());
}

/**
 * The tile size reaches the grid.
 **/
BOOST_AUTO_TEST_CASE(the_tile_size_is_passed_through) {
    const v3d::grid::Picture picture = v3d::grid::fromPicture({"."}, legend(), 2.0f);

    if (!picture.grid) {
        BOOST_ERROR("the picture was refused: " << picture.error);
        return;
    }
    const v3d::grid::TileGrid& grid = *picture.grid;
    BOOST_CHECK_EQUAL(grid.tileSize(), 2.0f);
}

BOOST_AUTO_TEST_SUITE_END()

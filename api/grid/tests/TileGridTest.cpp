/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/grid/TileCoord.h>
#include <api/grid/TileGrid.h>

#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

using v3d::grid::Cover;
using v3d::grid::TileCoord;
using v3d::grid::TileGrid;
using v3d::grid::toString;

namespace {

// the preprocessor splits a macro argument on every comma outside parentheses, so a braced
// TileCoord cannot be written inside a BOOST_CHECK. this is the same literal in a form the
// assertion macros can take
constexpr TileCoord at(int x, int y) {
    return TileCoord{ x, y };
}

// 4x4 at the default tile size spans 6 units, so its minimum corner sits at -3 on both axes
// and the world origin lands on the corner shared by tiles (1,1) and (2,2).
TileGrid makeGrid() {
    return TileGrid(4, 4);
}

bool near(float actual, float expected) {
    const float tolerance = 0.00001f;
    return (actual > expected ? actual - expected : expected - actual) < tolerance;
}

BOOST_AUTO_TEST_CASE(tilegrid_dimensions_test) {
    const TileGrid grid = makeGrid();

    BOOST_CHECK_EQUAL(grid.width(), 4);
    BOOST_CHECK_EQUAL(grid.height(), 4);
    BOOST_CHECK_EQUAL(near(grid.tileSize(), TileGrid::DEFAULT_TILE_SIZE), true);
}

BOOST_AUTO_TEST_CASE(tilegrid_degenerate_size_test) {
    BOOST_CHECK_THROW(TileGrid(0, 4), std::invalid_argument);
    BOOST_CHECK_THROW(TileGrid(4, -1), std::invalid_argument);
    BOOST_CHECK_THROW(TileGrid(4, 4, 0.0f), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(tilegrid_centred_on_origin_test) {
    const TileGrid grid = makeGrid();
    const glm::vec3 first = grid.tileToWorld(at(0, 0));
    const glm::vec3 last = grid.tileToWorld(at(3, 3));

    BOOST_CHECK_EQUAL(near(first.x, -2.25f), true);
    BOOST_CHECK_EQUAL(near(first.z, -2.25f), true);
    BOOST_CHECK_EQUAL(near(last.x, 2.25f), true);
    BOOST_CHECK_EQUAL(near(last.z, 2.25f), true);
}

BOOST_AUTO_TEST_CASE(tilegrid_tile_to_world_is_the_centre_test) {
    const TileGrid grid = makeGrid();
    const glm::vec3 centre = grid.tileToWorld(at(2, 1));

    // tile 2 starts at -3 + 2 * 1.5 = 0, so its centre is half a tile further on
    BOOST_CHECK_EQUAL(near(centre.x, 0.75f), true);
    BOOST_CHECK_EQUAL(near(centre.y, 0.0f), true);
    BOOST_CHECK_EQUAL(near(centre.z, -0.75f), true);
}

BOOST_AUTO_TEST_CASE(tilegrid_world_min_test) {
    const TileGrid grid = makeGrid();
    const glm::vec3 corner = grid.worldMin();

    BOOST_CHECK_EQUAL(near(corner.x, -3.0f), true);
    BOOST_CHECK_EQUAL(near(corner.y, 0.0f), true);
    BOOST_CHECK_EQUAL(near(corner.z, -3.0f), true);
}

BOOST_AUTO_TEST_CASE(tilegrid_centre_round_trips_test) {
    const TileGrid grid = makeGrid();

    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            const TileCoord tile{ x, y };
            BOOST_CHECK_EQUAL(grid.worldToTile(grid.tileToWorld(tile)) == tile, true);
        }
    }
}

BOOST_AUTO_TEST_CASE(tilegrid_world_to_tile_floors_test) {
    const TileGrid grid = makeGrid();

    // floors rather than rounds, so the mapping stays uniform either side of the origin
    BOOST_CHECK_EQUAL(grid.worldToTile(glm::vec3(-0.01f, 0.0f, -0.01f)) == at(1, 1), true);
    BOOST_CHECK_EQUAL(grid.worldToTile(glm::vec3(0.01f, 0.0f, 0.01f)) == at(2, 2), true);
}

BOOST_AUTO_TEST_CASE(tilegrid_shared_edge_belongs_to_the_higher_tile_test) {
    const TileGrid grid = makeGrid();

    BOOST_CHECK_EQUAL(grid.worldToTile(glm::vec3(0.0f, 0.0f, 0.0f)) == at(2, 2), true);
}

BOOST_AUTO_TEST_CASE(tilegrid_height_is_ignored_test) {
    const TileGrid grid = makeGrid();

    BOOST_CHECK_EQUAL(grid.worldToTile(glm::vec3(0.75f, 12.0f, -0.75f)) == at(2, 1), true);
}

BOOST_AUTO_TEST_CASE(tilegrid_off_grid_point_is_not_clamped_test) {
    const TileGrid grid = makeGrid();

    const TileCoord past = grid.worldToTile(glm::vec3(9.0f, 0.0f, 0.0f));
    BOOST_CHECK_EQUAL(past.x, 8);
    BOOST_CHECK_EQUAL(grid.contains(past), false);

    const TileCoord before = grid.worldToTile(glm::vec3(-3.01f, 0.0f, 0.0f));
    BOOST_CHECK_EQUAL(before.x, -1);
    BOOST_CHECK_EQUAL(grid.contains(before), false);
}

BOOST_AUTO_TEST_CASE(tilegrid_contains_test) {
    const TileGrid grid = makeGrid();

    BOOST_CHECK_EQUAL(grid.contains(at(0, 0)), true);
    BOOST_CHECK_EQUAL(grid.contains(at(3, 3)), true);
    BOOST_CHECK_EQUAL(grid.contains(at(-1, 0)), false);
    BOOST_CHECK_EQUAL(grid.contains(at(0, -1)), false);
    BOOST_CHECK_EQUAL(grid.contains(at(4, 0)), false);
    BOOST_CHECK_EQUAL(grid.contains(at(0, 4)), false);
}

BOOST_AUTO_TEST_CASE(tilegrid_passability_test) {
    TileGrid grid = makeGrid();

    BOOST_CHECK_EQUAL(grid.passable(at(1, 2)), true);

    grid.setPassable(at(1, 2), false);
    BOOST_CHECK_EQUAL(grid.passable(at(1, 2)), false);
    BOOST_CHECK_EQUAL(grid.passable(at(2, 1)), true);

    grid.setPassable(at(1, 2), true);
    BOOST_CHECK_EQUAL(grid.passable(at(1, 2)), true);
}

BOOST_AUTO_TEST_CASE(tilegrid_blocking_one_tile_does_not_alias_another_test) {
    TileGrid grid = makeGrid();
    grid.setPassable(at(3, 0), false);

    int blocked = 0;
    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            if (!grid.passable(at(x, y))) {
                ++blocked;
            }
        }
    }
    BOOST_CHECK_EQUAL(blocked, 1);
}

BOOST_AUTO_TEST_CASE(tilegrid_off_grid_is_impassable_rather_than_an_error_test) {
    const TileGrid grid = makeGrid();

    BOOST_CHECK_EQUAL(grid.passable(at(-1, 0)), false);
    BOOST_CHECK_EQUAL(grid.passable(at(4, 4)), false);
}

BOOST_AUTO_TEST_CASE(tilegrid_writing_off_the_map_throws_test) {
    TileGrid grid = makeGrid();

    BOOST_CHECK_THROW(grid.setPassable(at(4, 0), false), std::out_of_range);
    BOOST_CHECK_THROW(grid.setPassable(at(0, -1), false), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(tilegrid_starts_as_open_ground_test) {
    const TileGrid grid = makeGrid();

    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            BOOST_CHECK_EQUAL(grid.cover(at(x, y)) == Cover::None, true);
        }
    }
}

BOOST_AUTO_TEST_CASE(tilegrid_cover_reads_back_test) {
    TileGrid grid = makeGrid();

    grid.setCover(at(1, 2), Cover::Half);
    grid.setCover(at(3, 0), Cover::Full);

    BOOST_CHECK_EQUAL(grid.cover(at(1, 2)) == Cover::Half, true);
    BOOST_CHECK_EQUAL(grid.cover(at(3, 0)) == Cover::Full, true);
    BOOST_CHECK_EQUAL(grid.cover(at(2, 1)) == Cover::None, true);
}

BOOST_AUTO_TEST_CASE(tilegrid_cover_and_passability_are_independent_test) {
    TileGrid grid = makeGrid();

    // a doorway has no cover and is walked through; a crate is chest high and walked around
    grid.setCover(at(1, 1), Cover::Half);
    BOOST_CHECK_EQUAL(grid.passable(at(1, 1)), true);

    grid.setPassable(at(2, 2), false);
    BOOST_CHECK_EQUAL(grid.cover(at(2, 2)) == Cover::None, true);
}

BOOST_AUTO_TEST_CASE(tilegrid_nothing_off_the_map_has_cover_test) {
    TileGrid grid = makeGrid();

    BOOST_CHECK_EQUAL(grid.cover(at(-1, 0)) == Cover::None, true);
    BOOST_CHECK_EQUAL(grid.cover(at(4, 4)) == Cover::None, true);
    BOOST_CHECK_THROW(grid.setCover(at(-1, 0), Cover::Full), std::out_of_range);
    BOOST_CHECK_THROW(grid.setCover(at(4, 4), Cover::Full), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(tilegrid_every_cover_height_has_a_name_test) {
    BOOST_CHECK_EQUAL(toString(Cover::None), "none");
    BOOST_CHECK_EQUAL(toString(Cover::Half), "half");
    BOOST_CHECK_EQUAL(toString(Cover::Full), "full");
}

/**
 * A grid placed by its corner converts tiles from that corner rather than from the origin.
 **/
BOOST_AUTO_TEST_CASE(tilegrid_placed_by_its_corner_test) {
    const TileGrid grid(4, 4, 1.5f, glm::vec3(10.0f, 5.0f, -6.0f));

    // the corner's y is ignored, because the grid lies on Y = 0
    BOOST_CHECK(grid.worldMin() == glm::vec3(10.0f, 0.0f, -6.0f));
    const glm::vec3 first = grid.tileToWorld(at(0, 0));
    BOOST_CHECK(near(first.x, 10.75f));
    BOOST_CHECK(near(first.y, 0.0f));
    BOOST_CHECK(near(first.z, -5.25f));
    BOOST_CHECK(grid.worldToTile(glm::vec3(10.1f, 0.0f, -5.9f)) == at(0, 0));
    BOOST_CHECK(grid.worldToTile(glm::vec3(9.9f, 0.0f, -5.9f)) == at(-1, 0));
}

/**
 * Two grids placed side by side agree on their seam. A point on it belongs to the higher tile,
 * which is the second grid's first column. The first grid puts it just off its board.
 **/
BOOST_AUTO_TEST_CASE(tilegrid_side_by_side_grids_agree_on_their_seam_test) {
    const TileGrid west(4, 4, 1.0f, glm::vec3(0.0f));
    const TileGrid east(4, 4, 1.0f, glm::vec3(4.0f, 0.0f, 0.0f));

    const glm::vec3 seam(4.0f, 0.0f, 1.5f);
    BOOST_CHECK(west.worldToTile(seam) == at(4, 1));
    BOOST_CHECK(!west.contains(west.worldToTile(seam)));
    BOOST_CHECK(east.worldToTile(seam) == at(0, 1));

    const glm::vec3 justWest(3.999f, 0.0f, 1.5f);
    BOOST_CHECK(west.worldToTile(justWest) == at(3, 1));
    BOOST_CHECK(east.worldToTile(justWest) == at(-1, 1));
}

/**
 * Every tile of a grid placed at an awkward negative corner converts to the world and back to
 * itself.
 **/
BOOST_AUTO_TEST_CASE(tilegrid_round_trip_at_a_negative_corner_test) {
    const TileGrid grid(7, 5, 1.25f, glm::vec3(-100.5f, 0.0f, -37.25f));

    for (int y = 0; y < grid.height(); y++) {
        for (int x = 0; x < grid.width(); x++) {
            BOOST_CHECK(grid.worldToTile(grid.tileToWorld(at(x, y))) == at(x, y));
        }
    }
}

/**
 * A tile size or a corner that is not a finite number is refused, as a size that is not positive
 * is.
 **/
BOOST_AUTO_TEST_CASE(tilegrid_refuses_what_is_not_finite_test) {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();

    BOOST_CHECK_THROW(TileGrid(4, 4, nan), std::invalid_argument);
    BOOST_CHECK_THROW(TileGrid(4, 4, infinity), std::invalid_argument);
    BOOST_CHECK_THROW(TileGrid(4, 4, 1.0f, glm::vec3(nan, 0.0f, 0.0f)), std::invalid_argument);
    BOOST_CHECK_THROW(TileGrid(4, 4, 1.0f, glm::vec3(0.0f, 0.0f, -infinity)), std::invalid_argument);
    // the y the grid ignores may be anything
    BOOST_CHECK_NO_THROW(TileGrid(4, 4, 1.0f, glm::vec3(0.0f, nan, 0.0f)));
}

/**
 * A tile size large enough that a side of the grid overflows a float is refused for its size,
 * not for the corner a centred grid works out from it.
 **/
BOOST_AUTO_TEST_CASE(tilegrid_refuses_a_side_too_long_to_hold_test) {
    const float huge = std::numeric_limits<float>::max() / 2.0f;

    const std::optional<std::string> refused = TileGrid::refusal(10, 1, huge, glm::vec3(0.0f));
    if (!refused) {
        BOOST_ERROR("a side too long to hold was not refused");
        return;
    }
    BOOST_CHECK(refused->find("tile size") != std::string::npos);
    BOOST_CHECK_THROW(TileGrid(10, 1, huge), std::invalid_argument);
    BOOST_CHECK(!TileGrid::refusal(10, 1, 1.0f, glm::vec3(0.0f)));
}

};  // namespace

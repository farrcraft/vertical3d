/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/grid/DistanceField.h>
#include <api/grid/GridSet.h>
#include <api/grid/LineOfSight.h>
#include <api/grid/Pathfinding.h>
#include <api/grid/ReachableTile.h>
#include <api/grid/Sight.h>
#include <api/grid/TileCoord.h>
#include <api/grid/TileGrid.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <glm/vec3.hpp>

using v3d::grid::Cover;
using v3d::grid::DistanceField;
using v3d::grid::GridSet;
using v3d::grid::ReachableTile;
using v3d::grid::Sight;
using v3d::grid::TileCoord;
using v3d::grid::TileGrid;

namespace {

constexpr TileCoord at(int x, int y) {
    return TileCoord{ x, y };
}

/**
 * A grid of tiles one unit across, placed with its tile (0, 0) at world tile corner. The rows
 * are given top first, as they are drawn: the first is the highest y. '#' is a wall, which
 * blocks movement and sight, and anything else is open floor.
 **/
TileGrid board(const std::vector<std::string>& rows, TileCoord corner) {
    const int height = static_cast<int>(rows.size());
    const int width = static_cast<int>(rows.front().size());
    TileGrid grid(width, height, 1.0F, glm::vec3(static_cast<float>(corner.x), 0.0F, static_cast<float>(corner.y)));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (rows[static_cast<std::size_t>(height - 1 - y)][static_cast<std::size_t>(x)] == '#') {
                grid.setPassable(at(x, y), false);
                grid.setCover(at(x, y), Cover::Full);
            }
        }
    }
    return grid;
}

/**
 * Two regions side by side, west at world (0, 0) and east at (4, 0). S is world (0, 1) and G is
 * world (7, 1). West's wall on row 1 faces east's across the seam, and west's row 0 is walled off
 * from the rest of west.
 *
 *     west              east
 * y=2   . . . .           . . . .
 * y=1   S . . #           # . . G
 * y=0   # # # .           . . . .
 **/
std::vector<std::string> westRows() {
    return { "....", "...#", "###." };
}

std::vector<std::string> eastRows() {
    return { "....", "#...", "...." };
}

GridSet both(const TileGrid& first, const TileGrid& second) {
    GridSet set;
    set.add(first);
    set.add(second);
    return set;
}

bool passesThrough(const std::vector<TileCoord>& path, TileCoord tile) {
    return std::ranges::find(path, tile) != path.end();
}

/**
 * Whether a refusal was given, and says what it should.
 **/
bool refusedFor(const std::optional<std::string>& refusal, const std::string& reason) {
    return refusal && refusal->find(reason) != std::string::npos;
}

/**
 * Whether every step of a path is one 8-way step, so a route is a walk and not a jump.
 **/
bool contiguous(const std::vector<TileCoord>& path) {
    for (std::size_t i = 1; i < path.size(); ++i) {
        if (v3d::grid::tileDistance(path[i - 1], path[i]) != 1) {
            return false;
        }
    }
    return true;
}

BOOST_AUTO_TEST_SUITE(grid_set_test)

/**
 * A route crosses the seam on row 2, between the two walls that face each other, and is as short
 * as the route over one grid holding the same tiles.
 **/
BOOST_AUTO_TEST_CASE(a_route_crosses_a_seam_as_over_one_grid) {
    const TileGrid west = board(westRows(), at(0, 0));
    const TileGrid east = board(eastRows(), at(4, 0));
    const GridSet set = both(west, east);

    const std::vector<TileCoord> path = v3d::grid::findPath(set, at(0, 1), at(7, 1));
    BOOST_REQUIRE(!path.empty());
    BOOST_CHECK(path.front() == at(0, 1));
    BOOST_CHECK(path.back() == at(7, 1));
    BOOST_CHECK(contiguous(path));
    BOOST_CHECK(passesThrough(path, at(3, 2)) || passesThrough(path, at(4, 2)));

    const TileGrid whole = board({ "........", "...##...", "###....." }, at(0, 0));
    const std::vector<TileCoord> single = v3d::grid::findPath(whole, at(0, 1), at(7, 1));
    BOOST_CHECK_EQUAL(path.size(), single.size());
}

/**
 * World (3, 0) is walled off from the rest of west. It is reached only by leaving west, going
 * round east's wall through east's (1, 1), world (5, 1), and coming back into west on row 0. A
 * search of west alone finds no route, so two routes joined at the seam cannot find this one.
 **/
BOOST_AUTO_TEST_CASE(a_route_may_leave_a_grid_and_come_back) {
    const TileGrid west = board(westRows(), at(0, 0));
    const TileGrid east = board(eastRows(), at(4, 0));
    const GridSet set = both(west, east);

    const std::vector<TileCoord> path = v3d::grid::findPath(set, at(0, 1), at(3, 0));
    BOOST_REQUIRE(!path.empty());
    BOOST_CHECK(contiguous(path));
    BOOST_CHECK(passesThrough(path, at(5, 1)));
    BOOST_CHECK(passesThrough(path, at(4, 0)));

    BOOST_CHECK(v3d::grid::findPath(west, at(0, 1), at(3, 0)).empty());
}

/**
 * A diagonal may not pass between two blocked tiles that lie in different grids. With west's
 * wall moved up to (3, 2), consider the step from west's (3, 1) to east's (0, 2), world (4, 2).
 * It would pass between west's wall and east's, so it is refused and the route goes the long
 * way.
 **/
BOOST_AUTO_TEST_CASE(a_diagonal_between_walls_in_two_grids_is_refused) {
    const TileGrid west = board({ "...#", "....", "###." }, at(0, 0));
    const TileGrid east = board(eastRows(), at(4, 0));
    const GridSet set = both(west, east);

    const std::vector<ReachableTile> near = v3d::grid::reachableTiles(set, at(3, 1), 1);
    BOOST_CHECK(std::ranges::none_of(near, [](const ReachableTile& tile) { return tile.tile == at(4, 2); }));
    const std::vector<TileCoord> path = v3d::grid::findPath(set, at(3, 1), at(4, 2));
    BOOST_REQUIRE(!path.empty());
    BOOST_CHECK_GT(path.size(), 2U);
}

/**
 * A set of one grid gives the route findPath() gives on that grid, tie for tie, whether it is
 * the grid alone in its own coordinates or a grid at the world's origin.
 **/
BOOST_AUTO_TEST_CASE(a_set_of_one_grid_routes_as_the_grid_does) {
    const TileGrid grid = board({ ".....", ".#.#.", ".....", "..#.." }, at(0, 0));
    GridSet placed;
    placed.add(grid);
    for (const TileCoord goal : { at(4, 3), at(2, 2), at(0, 0) }) {
        BOOST_TEST_CONTEXT("goal " << goal.x << ", " << goal.y) {
            const std::vector<TileCoord> alone = v3d::grid::findPath(grid, at(0, 3), goal);
            BOOST_CHECK(v3d::grid::findPath(GridSet::local(grid), at(0, 3), goal) == alone);
            BOOST_CHECK(v3d::grid::findPath(placed, at(0, 3), goal) == alone);
        }
    }
}

/**
 * A tile in no grid of the set is impassable: a goal there has no route, and nor has a start.
 * A region that is not loaded is therefore never routed into.
 **/
BOOST_AUTO_TEST_CASE(a_tile_in_no_grid_has_no_route) {
    const TileGrid west = board(westRows(), at(0, 0));
    GridSet set;
    set.add(west);
    BOOST_CHECK(v3d::grid::findPath(set, at(0, 1), at(5, 1)).empty());
    BOOST_CHECK(v3d::grid::findPath(set, at(5, 1), at(0, 1)).empty());
    BOOST_CHECK(!set.passable(at(5, 1)));
    BOOST_CHECK(set.opaque(at(5, 1)));
}

/**
 * Reachable tiles and a distance field cross a seam, and agree with the route. The field keeps
 * where the grids lie but reads none of them, so it answers after the grids are gone.
 **/
BOOST_AUTO_TEST_CASE(reachable_tiles_and_a_distance_field_cross_a_seam) {
    std::optional<DistanceField> field;
    std::size_t routeCost = 0;
    {
        const TileGrid west = board(westRows(), at(0, 0));
        const TileGrid east = board(eastRows(), at(4, 0));
        const GridSet set = both(west, east);
        routeCost = v3d::grid::findPath(set, at(0, 1), at(7, 1)).size() - 1;

        const std::vector<ReachableTile> reached = v3d::grid::reachableTiles(set, at(0, 1), static_cast<int>(routeCost));
        const auto goal = std::ranges::find_if(reached, [](const ReachableTile& tile) { return tile.tile == at(7, 1); });
        BOOST_REQUIRE(goal != reached.end());
        BOOST_CHECK_EQUAL(static_cast<std::size_t>(goal->cost), routeCost);

        field.emplace(set, at(7, 1));
    }
    BOOST_CHECK_EQUAL(static_cast<std::size_t>(field->cost(at(0, 1))), routeCost);
    BOOST_CHECK_EQUAL(field->cost(at(7, 1)), 0);
    BOOST_CHECK_EQUAL(field->cost(at(9, 1)), DistanceField::UNREACHABLE);
    BOOST_CHECK(!field->reaches(at(-1, 0)));
}

/**
 * Sight across a seam is symmetric, as within one grid. The two walls facing each other on row 1
 * block the line between them from either end, and a line that leaves the grids is stopped.
 **/
BOOST_AUTO_TEST_CASE(sight_across_a_seam_is_symmetric) {
    const TileGrid west = board(westRows(), at(0, 0));
    const TileGrid east = board(eastRows(), at(4, 0));
    const GridSet set = both(west, east);

    const std::vector<TileCoord> tiles = { at(0, 1), at(1, 2), at(2, 1), at(3, 0), at(5, 1), at(6, 2), at(7, 0), at(7, 1) };
    for (const TileCoord from : tiles) {
        for (const TileCoord to : tiles) {
            BOOST_TEST_CONTEXT("from " << from.x << ", " << from.y << " to " << to.x << ", " << to.y) {
                BOOST_CHECK_EQUAL(v3d::grid::hasLineOfSight(set, from, to), v3d::grid::hasLineOfSight(set, to, from));
            }
        }
    }
    BOOST_CHECK(v3d::grid::hasLineOfSight(set, at(1, 2), at(6, 2)));
    BOOST_CHECK(!v3d::grid::hasLineOfSight(set, at(2, 1), at(5, 1)));
    BOOST_CHECK(!v3d::grid::hasLineOfSight(set, at(5, 1), at(2, 1)));

    GridSet westOnly;
    westOnly.add(west);
    BOOST_CHECK(!v3d::grid::hasLineOfSight(westOnly, at(1, 2), at(6, 2)));
}

/**
 * A viewer standing by the seam sees into the grid beside it. Each grid's Sight is looked at in
 * turn from the viewer's world tile, and answers in its own coordinates.
 **/
BOOST_AUTO_TEST_CASE(sight_sees_across_a_seam) {
    const TileGrid west = board(westRows(), at(0, 0));
    const TileGrid east = board(eastRows(), at(4, 0));
    const GridSet set = both(west, east);

    Sight seenOfA(7);
    Sight seenOfB(7);
    seenOfA.look(set, west, at(2, 2));
    seenOfB.look(set, east, at(2, 2));
    BOOST_CHECK(seenOfA.visible(at(2, 2)));
    BOOST_CHECK(seenOfB.visible(at(2, 2)));
    BOOST_CHECK(seenOfB.remembered(at(3, 2)));

    // a board that is not in the set has nothing in sight
    const TileGrid elsewhere = board(eastRows(), at(20, 0));
    Sight seenOfElsewhere(7);
    seenOfElsewhere.look(set, elsewhere, at(2, 2));
    BOOST_CHECK(!seenOfElsewhere.visible(at(0, 0)));
}

/**
 * A grid joins a set only when it shares the set's tile size, lies on the world's tile lattice
 * and overlaps no grid already there. Each refusal says why, and add() throws it.
 **/
BOOST_AUTO_TEST_CASE(a_grid_that_does_not_fit_the_set_is_refused) {
    const TileGrid west = board(westRows(), at(0, 0));
    GridSet set;
    set.add(west);

    const TileGrid larger(4, 3, 2.0F, glm::vec3(8.0F, 0.0F, 0.0F));
    BOOST_CHECK(refusedFor(set.refusal(larger), "tiles 2 across cannot join a set of tiles 1 across"));

    const TileGrid shifted(4, 3, 1.0F, glm::vec3(4.5F, 0.0F, 0.0F));
    BOOST_CHECK(refusedFor(set.refusal(shifted), "not a whole number of tiles"));

    const TileGrid overlapping(4, 3, 1.0F, glm::vec3(2.0F, 0.0F, 1.0F));
    BOOST_CHECK(refusedFor(set.refusal(overlapping), "overlaps"));
    BOOST_CHECK_THROW(set.add(overlapping), std::invalid_argument);

    const TileGrid beside = board(eastRows(), at(4, 0));
    BOOST_CHECK(!set.refusal(beside));
    BOOST_CHECK(set.origin(west) == at(0, 0));
    set.add(beside);
    BOOST_CHECK(set.origin(beside) == at(4, 0));
    BOOST_CHECK(set.gridAt(at(5, 2)) == &beside);
    BOOST_CHECK_EQUAL(set.count(), 24U);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace

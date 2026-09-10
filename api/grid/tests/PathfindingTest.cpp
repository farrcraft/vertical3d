/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/grid/Pathfinding.h>
#include <api/grid/TileGrid.h>

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

using v3d::grid::Cover;
using v3d::grid::DistanceField;
using v3d::grid::ReachableTile;
using v3d::grid::TileCoord;
using v3d::grid::TileFilter;
using v3d::grid::TileGrid;
using v3d::grid::findPath;
using v3d::grid::reachableTiles;
using v3d::grid::tileDistance;

namespace {

// the preprocessor splits a macro argument on every comma outside parentheses, so a braced
// TileCoord cannot be written inside a BOOST_CHECK. this is the same literal in a form the
// assertion macros can take
constexpr TileCoord at(int x, int y) {
    return TileCoord{ x, y };
}

// odd sizes so a start tile can sit exactly in the middle and a budget reaches the same
// distance in every direction without clipping
TileGrid makeGrid() {
    return TileGrid(9, 9);
}

constexpr TileCoord CENTRE{ 4, 4 };

// what a flood fill reached a tile at, or -1 if it did not reach it
int costOf(const std::vector<ReachableTile>& reached, TileCoord tile) {
    for (const ReachableTile& entry : reached) {
        if (entry.tile == tile) {
            return entry.cost;
        }
    }
    return -1;
}

bool contains(const std::vector<TileCoord>& tiles, TileCoord tile) {
    return std::ranges::find(tiles, tile) != tiles.end();
}

// whether every consecutive pair in a path is one 8 way step apart. a path that teleports is
// the failure mode a length assertion alone would not catch
bool contiguous(const std::vector<TileCoord>& path) {
    for (std::size_t i = 1; i < path.size(); ++i) {
        if (tileDistance(path[i - 1], path[i]) != 1) {
            return false;
        }
    }
    return true;
}

// a filter that blocks a fixed set of tiles, standing in for an occupancy map
TileFilter blocking(std::vector<TileCoord> occupied) {
    return [occupied = std::move(occupied)](TileCoord tile) {
        return !contains(occupied, tile);
    };
}

};  // namespace

BOOST_AUTO_TEST_CASE(tiledistance_is_chebyshev_test) {
    BOOST_CHECK_EQUAL(tileDistance(at(0, 0), at(0, 0)), 0);
    BOOST_CHECK_EQUAL(tileDistance(at(0, 0), at(3, 0)), 3);
    BOOST_CHECK_EQUAL(tileDistance(at(0, 0), at(0, 3)), 3);
    BOOST_CHECK_EQUAL(tileDistance(at(0, 0), at(3, 3)), 3);
    BOOST_CHECK_EQUAL(tileDistance(at(0, 0), at(2, 5)), 5);
    BOOST_CHECK_EQUAL(tileDistance(at(5, 2), at(0, 0)), 5);
}

BOOST_AUTO_TEST_CASE(findpath_to_where_you_stand_is_that_tile_alone_test) {
    const TileGrid grid = makeGrid();

    const std::vector<TileCoord> path = findPath(grid, CENTRE, CENTRE);

    BOOST_CHECK_EQUAL(path.size(), 1u);
    BOOST_CHECK_EQUAL(path.front() == CENTRE, true);
}

BOOST_AUTO_TEST_CASE(findpath_orthogonal_run_test) {
    const TileGrid grid = makeGrid();

    const std::vector<TileCoord> path = findPath(grid, at(0, 0), at(4, 0));

    BOOST_CHECK_EQUAL(path.size(), 5u);
    BOOST_CHECK_EQUAL(path.front() == at(0, 0), true);
    BOOST_CHECK_EQUAL(path.back() == at(4, 0), true);
    BOOST_CHECK_EQUAL(contiguous(path), true);
}

BOOST_AUTO_TEST_CASE(findpath_diagonal_run_costs_one_per_tile_test) {
    const TileGrid grid = makeGrid();

    const std::vector<TileCoord> path = findPath(grid, at(0, 0), at(4, 4));

    // four steps, not eight: this is the whole of the 8 way decision in one assertion
    BOOST_CHECK_EQUAL(path.size(), 5u);
    BOOST_CHECK_EQUAL(path.back() == at(4, 4), true);
    BOOST_CHECK_EQUAL(contiguous(path), true);
}

BOOST_AUTO_TEST_CASE(findpath_off_axis_goal_costs_the_longer_axis_test) {
    const TileGrid grid = makeGrid();

    const std::vector<TileCoord> path = findPath(grid, at(0, 0), at(5, 2));

    BOOST_CHECK_EQUAL(path.size(), 6u);
    BOOST_CHECK_EQUAL(path.back() == at(5, 2), true);
    BOOST_CHECK_EQUAL(contiguous(path), true);
}

BOOST_AUTO_TEST_CASE(findpath_routes_around_an_impassable_tile_test) {
    TileGrid grid = makeGrid();
    grid.setPassable(at(1, 0), false);

    const std::vector<TileCoord> path = findPath(grid, at(0, 0), at(2, 0));

    BOOST_CHECK_EQUAL(path.size(), 3u);
    BOOST_CHECK_EQUAL(path.back() == at(2, 0), true);
    BOOST_CHECK_EQUAL(contiguous(path), true);
    BOOST_CHECK_EQUAL(contains(path, at(1, 0)), false);
}

BOOST_AUTO_TEST_CASE(findpath_rounding_an_offset_wall_is_free_test) {
    TileGrid grid = makeGrid();
    for (int y = 0; y < 8; ++y) {
        grid.setPassable(at(4, y), false);
    }

    const std::vector<TileCoord> path = findPath(grid, at(0, 4), at(8, 4));

    BOOST_CHECK_EQUAL(path.empty(), false);
    BOOST_CHECK_EQUAL(path.back() == at(8, 4), true);
    BOOST_CHECK_EQUAL(contiguous(path), true);
    BOOST_CHECK_EQUAL(std::ranges::none_of(path, [](TileCoord t) { return t.x == 4 && t.y < 8; }), true);

    // straight across is 8 steps, and going around by way of the gap at (4, 8) is also 8:
    // the four rows of detour are absorbed by the four columns of travel it already had to
    // make. this is the Chebyshev under-charge that 8 way movement at a flat cost buys - a
    // wall only costs something when the way round leaves the diagonal envelope
    BOOST_CHECK_EQUAL(path.size(), 9u);
}

BOOST_AUTO_TEST_CASE(findpath_a_wall_costs_when_the_way_round_leaves_the_envelope_test) {
    TileGrid grid = makeGrid();
    for (int y = 1; y < 9; ++y) {
        grid.setPassable(at(4, y), false);
    }

    // two tiles apart, but the only gap in the wall between them is at (4, 0)
    const std::vector<TileCoord> path = findPath(grid, at(3, 4), at(5, 4));

    BOOST_CHECK_EQUAL(path.empty(), false);
    BOOST_CHECK_EQUAL(path.back() == at(5, 4), true);
    BOOST_CHECK_EQUAL(contiguous(path), true);
    BOOST_CHECK_EQUAL(contains(path, at(4, 0)), true);

    // four steps up to the gap and four back down, against two in a straight line
    BOOST_CHECK_EQUAL(path.size(), 9u);
}

BOOST_AUTO_TEST_CASE(findpath_an_enclosed_goal_has_no_path_test) {
    TileGrid grid = makeGrid();
    for (int y = 3; y <= 5; ++y) {
        for (int x = 3; x <= 5; ++x) {
            if (!(at(x, y) == CENTRE)) {
                grid.setPassable(at(x, y), false);
            }
        }
    }

    BOOST_CHECK_EQUAL(findPath(grid, at(0, 0), CENTRE).empty(), true);
}

BOOST_AUTO_TEST_CASE(findpath_an_impassable_goal_has_no_path_test) {
    TileGrid grid = makeGrid();
    grid.setPassable(at(4, 0), false);

    BOOST_CHECK_EQUAL(findPath(grid, at(0, 0), at(4, 0)).empty(), true);
}

BOOST_AUTO_TEST_CASE(findpath_an_off_grid_goal_has_no_path_test) {
    const TileGrid grid = makeGrid();

    BOOST_CHECK_EQUAL(findPath(grid, at(0, 0), at(9, 0)).empty(), true);
    BOOST_CHECK_EQUAL(findPath(grid, at(0, 0), at(-1, 0)).empty(), true);
}

BOOST_AUTO_TEST_CASE(findpath_an_off_grid_start_has_no_path_test) {
    const TileGrid grid = makeGrid();

    BOOST_CHECK_EQUAL(findPath(grid, at(-1, 0), at(0, 0)).empty(), true);
}

BOOST_AUTO_TEST_CASE(findpath_the_start_is_never_tested_test) {
    TileGrid grid = makeGrid();
    grid.setPassable(CENTRE, false);

    // the mover stands on the start tile; whatever makes it unenterable does not trap
    // whoever is already there
    const std::vector<TileCoord> path = findPath(grid, CENTRE, at(6, 4), blocking({ CENTRE }));

    BOOST_CHECK_EQUAL(path.size(), 3u);
    BOOST_CHECK_EQUAL(path.front() == CENTRE, true);
    BOOST_CHECK_EQUAL(path.back() == at(6, 4), true);
}

BOOST_AUTO_TEST_CASE(findpath_the_filter_blocks_tiles_the_grid_calls_passable_test) {
    const TileGrid grid = makeGrid();

    const std::vector<TileCoord> path =
        findPath(grid, at(0, 0), at(2, 0), blocking({ at(1, 0), at(1, 1) }));

    BOOST_CHECK_EQUAL(path.empty(), false);
    BOOST_CHECK_EQUAL(path.back() == at(2, 0), true);
    BOOST_CHECK_EQUAL(contiguous(path), true);
    BOOST_CHECK_EQUAL(contains(path, at(1, 0)), false);
    BOOST_CHECK_EQUAL(contains(path, at(1, 1)), false);
}

BOOST_AUTO_TEST_CASE(findpath_a_goal_the_filter_blocks_has_no_path_test) {
    const TileGrid grid = makeGrid();

    BOOST_CHECK_EQUAL(
        findPath(grid, at(0, 0), at(2, 2), blocking({ at(2, 2) })).empty(), true);
}

BOOST_AUTO_TEST_CASE(findpath_cannot_squeeze_between_two_blocked_tiles_test) {
    TileGrid grid = makeGrid();

    // a wall laid corner to corner across the centre. passing from (3,3) to (4,4) would mean
    // slipping between (4,3) and (3,4), which is what the corner rule forbids
    grid.setPassable(at(4, 3), false);
    grid.setPassable(at(3, 4), false);

    const std::vector<TileCoord> path = findPath(grid, at(3, 3), at(4, 4));

    BOOST_CHECK_EQUAL(path.empty(), false);
    BOOST_CHECK_EQUAL(path.back() == at(4, 4), true);
    BOOST_CHECK_EQUAL(contiguous(path), true);
    BOOST_CHECK_EQUAL(path.size() > 2, true);
}

BOOST_AUTO_TEST_CASE(findpath_the_corner_rule_reads_the_filter_too_test) {
    const TileGrid grid = makeGrid();

    // the same rule, sourced from the filter rather than from the grid: a wall of bodies is
    // a wall
    const std::vector<TileCoord> path =
        findPath(grid, at(3, 3), at(4, 4), blocking({ at(4, 3), at(3, 4) }));

    BOOST_CHECK_EQUAL(path.empty(), false);
    BOOST_CHECK_EQUAL(path.size() > 2, true);
}

BOOST_AUTO_TEST_CASE(findpath_a_single_blocked_corner_can_still_be_cut_test) {
    TileGrid grid = makeGrid();
    grid.setPassable(at(4, 3), false);

    // only one of the two tiles the diagonal passes between is blocked, so rounding the end
    // of the wall passes through nothing and is allowed
    const std::vector<TileCoord> path = findPath(grid, at(3, 3), at(4, 4));

    BOOST_CHECK_EQUAL(path.size(), 2u);
    BOOST_CHECK_EQUAL(path.back() == at(4, 4), true);
}

BOOST_AUTO_TEST_CASE(reachabletiles_a_budget_of_nothing_reaches_where_you_stand_test) {
    const TileGrid grid = makeGrid();

    const std::vector<ReachableTile> reached = reachableTiles(grid, CENTRE, 0);

    BOOST_CHECK_EQUAL(reached.size(), 1u);
    BOOST_CHECK_EQUAL(reached.front().tile == CENTRE, true);
    BOOST_CHECK_EQUAL(reached.front().cost, 0);
}

BOOST_AUTO_TEST_CASE(reachabletiles_an_open_budget_reaches_a_square_test) {
    const TileGrid grid = makeGrid();

    // Chebyshev distance, so the set is a square rather than a diamond. 2 * 2 + 1 = 5
    BOOST_CHECK_EQUAL(reachableTiles(grid, CENTRE, 2).size(), 25u);
    BOOST_CHECK_EQUAL(reachableTiles(grid, CENTRE, 3).size(), 49u);
}

BOOST_AUTO_TEST_CASE(reachabletiles_cost_is_chebyshev_on_open_ground_test) {
    const TileGrid grid = makeGrid();

    const std::vector<ReachableTile> reached = reachableTiles(grid, CENTRE, 3);

    for (const ReachableTile& tile : reached) {
        BOOST_CHECK_EQUAL(tile.cost, tileDistance(CENTRE, tile.tile));
    }
}

BOOST_AUTO_TEST_CASE(reachabletiles_is_clipped_by_the_edge_test) {
    const TileGrid grid = makeGrid();

    // a 5x5 square anchored in the corner keeps only the quadrant that is on the map
    const std::vector<ReachableTile> reached = reachableTiles(grid, at(0, 0), 2);

    BOOST_CHECK_EQUAL(reached.size(), 9u);
    BOOST_CHECK_EQUAL(costOf(reached, at(-1, 0)), -1);
}

BOOST_AUTO_TEST_CASE(reachabletiles_excludes_impassable_tiles_test) {
    TileGrid grid = makeGrid();
    grid.setPassable(at(5, 4), false);

    const std::vector<ReachableTile> reached = reachableTiles(grid, CENTRE, 2);

    BOOST_CHECK_EQUAL(reached.size(), 24u);
    BOOST_CHECK_EQUAL(costOf(reached, at(5, 4)), -1);
}

BOOST_AUTO_TEST_CASE(reachabletiles_excludes_tiles_the_filter_blocks_test) {
    const TileGrid grid = makeGrid();

    const std::vector<ReachableTile> reached =
        reachableTiles(grid, CENTRE, 2, blocking({ at(5, 4), at(3, 4) }));

    BOOST_CHECK_EQUAL(reached.size(), 23u);
    BOOST_CHECK_EQUAL(costOf(reached, at(5, 4)), -1);
    BOOST_CHECK_EQUAL(costOf(reached, at(3, 4)), -1);
}

BOOST_AUTO_TEST_CASE(reachabletiles_a_tile_behind_a_wall_costs_the_way_around_test) {
    TileGrid grid = makeGrid();
    for (int y = 2; y <= 6; ++y) {
        grid.setPassable(at(5, y), false);
    }

    const std::vector<ReachableTile> reached = reachableTiles(grid, CENTRE, 8);

    // straight through is 2 steps, but the wall spans y 2..6, so the cheapest crossing is at
    // y = 1: three steps up to (5, 1) and three back down to (6, 4)
    BOOST_CHECK_EQUAL(costOf(reached, at(6, 4)), 6);
}

BOOST_AUTO_TEST_CASE(reachabletiles_a_short_budget_does_not_reach_the_tile_test) {
    TileGrid grid = makeGrid();
    for (int y = 2; y <= 6; ++y) {
        grid.setPassable(at(5, y), false);
    }

    // the same tile, two steps away in a straight line and six by the only route there is.
    // a highlight must not offer it for less than the move would charge
    BOOST_CHECK_EQUAL(costOf(reachableTiles(grid, CENTRE, 4), at(6, 4)), -1);
}

BOOST_AUTO_TEST_CASE(reachabletiles_a_walled_off_tile_is_not_reached_test) {
    TileGrid grid = makeGrid();
    for (int y = 3; y <= 5; ++y) {
        for (int x = 3; x <= 5; ++x) {
            if (!(at(x, y) == CENTRE)) {
                grid.setPassable(at(x, y), false);
            }
        }
    }

    const std::vector<ReachableTile> reached = reachableTiles(grid, CENTRE, 8);

    BOOST_CHECK_EQUAL(reached.size(), 1u);
    BOOST_CHECK_EQUAL(reached.front().tile == CENTRE, true);
}

BOOST_AUTO_TEST_CASE(reachabletiles_an_off_grid_start_reaches_nothing_test) {
    const TileGrid grid = makeGrid();

    BOOST_CHECK_EQUAL(reachableTiles(grid, at(-1, 0), 3).empty(), true);
}

BOOST_AUTO_TEST_CASE(reachabletiles_rejects_a_negative_budget_test) {
    const TileGrid grid = makeGrid();

    BOOST_CHECK_THROW(reachableTiles(grid, CENTRE, -1), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(reachabletiles_the_corner_rule_charges_what_the_path_does_test) {
    TileGrid grid = makeGrid();

    // the highlight and the path search read one predicate: if they ever disagree, the
    // highlight offers a tile the move then refuses
    grid.setCover(at(4, 3), Cover::Full);
    grid.setCover(at(3, 4), Cover::Full);
    grid.setPassable(at(4, 3), false);
    grid.setPassable(at(3, 4), false);

    const int cost = costOf(reachableTiles(grid, at(3, 3), 4), at(4, 4));

    BOOST_CHECK_EQUAL(cost > 1, true);
    BOOST_CHECK_EQUAL(cost, static_cast<int>(findPath(grid, at(3, 3), at(4, 4)).size()) - 1);
}

BOOST_AUTO_TEST_CASE(distancefield_agrees_with_tiledistance_on_open_ground_test) {
    const TileGrid grid = makeGrid();
    const DistanceField field(grid, CENTRE);

    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            const TileCoord tile{ x, y };
            BOOST_CHECK_EQUAL(field.cost(tile), tileDistance(tile, CENTRE));
        }
    }
}

BOOST_AUTO_TEST_CASE(distancefield_costs_what_findpath_charges_test) {
    TileGrid grid = makeGrid();

    // a wall across the middle of the board with one hole in it: the tile against the wall
    // is the closest one by tileDistance and the furthest one from anywhere that leads round
    for (int x = 0; x < 8; ++x) {
        grid.setPassable(at(x, 4), false);
    }

    const DistanceField field(grid, at(0, 8));

    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            const TileCoord tile{ x, y };
            if (!grid.passable(tile)) {
                continue;
            }
            const std::vector<TileCoord> route = findPath(grid, tile, at(0, 8));
            BOOST_CHECK_EQUAL(field.cost(tile), static_cast<int>(route.size()) - 1);
        }
    }
}

BOOST_AUTO_TEST_CASE(distancefield_the_tile_against_a_wall_is_further_than_the_way_round_test) {
    TileGrid grid = makeGrid();
    for (int x = 0; x < 8; ++x) {
        grid.setPassable(at(x, 4), false);
    }

    const DistanceField field(grid, at(0, 8));

    // (0,3) is five tiles from the goal as the crow flies, and stepping down the column
    // toward it is what a greedy approach does; the only way round is the hole at the far end
    // of the wall, which every one of those steps walks away from
    BOOST_CHECK_EQUAL(tileDistance(at(0, 3), at(0, 8)), 5);
    BOOST_CHECK_EQUAL(field.cost(at(0, 3)) > field.cost(at(7, 3)), true);
}

BOOST_AUTO_TEST_CASE(distancefield_a_walled_off_tile_is_unreachable_test) {
    TileGrid grid = makeGrid();
    grid.setPassable(at(0, 1), false);
    grid.setPassable(at(1, 1), false);
    grid.setPassable(at(1, 0), false);

    const DistanceField field(grid, CENTRE);

    BOOST_CHECK_EQUAL(field.reaches(at(0, 0)), false);
    BOOST_CHECK_EQUAL(field.cost(at(0, 0)), DistanceField::UNREACHABLE);
    BOOST_CHECK_EQUAL(field.reaches(CENTRE), true);
}

BOOST_AUTO_TEST_CASE(distancefield_the_goal_is_never_tested_test) {
    const TileGrid grid = makeGrid();

    // whoever is standing on the goal does not make it unreachable - the same exemption
    // findPath() gives its start, and what lets an approach measure against an occupied tile
    const DistanceField field(grid, CENTRE, blocking({ CENTRE }));

    BOOST_CHECK_EQUAL(field.cost(CENTRE), 0);
    BOOST_CHECK_EQUAL(field.cost(at(4, 3)), 1);
}

BOOST_AUTO_TEST_CASE(distancefield_a_filtered_tile_is_walked_round_test) {
    TileGrid grid = makeGrid();
    for (int x = 0; x < 8; ++x) {
        grid.setPassable(at(x, 4), false);
    }

    // the hole at the end of the wall, blocked by the filter rather than by the terrain
    const DistanceField field(grid, at(0, 8), blocking({ at(8, 4) }));

    BOOST_CHECK_EQUAL(field.reaches(at(0, 3)), false);
}

BOOST_AUTO_TEST_CASE(distancefield_an_off_grid_goal_reaches_nothing_test) {
    const TileGrid grid = makeGrid();
    const DistanceField field(grid, at(-1, 0));

    BOOST_CHECK_EQUAL(field.reaches(CENTRE), false);
}

BOOST_AUTO_TEST_CASE(distancefield_an_off_grid_tile_is_unreachable_test) {
    const TileGrid grid = makeGrid();
    const DistanceField field(grid, CENTRE);

    BOOST_CHECK_EQUAL(field.reaches(at(-1, 4)), false);
    BOOST_CHECK_EQUAL(field.reaches(at(9, 4)), false);
    BOOST_CHECK_EQUAL(field.reaches(at(4, -1)), false);
    BOOST_CHECK_EQUAL(field.reaches(at(4, 9)), false);
}

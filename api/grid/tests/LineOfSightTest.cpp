/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "../LineOfSight.h"
#include "../Pathfinding.h"
#include "../TileGrid.h"

using v3d::grid::Cover;
using v3d::grid::SightBlocker;
using v3d::grid::TileCoord;
using v3d::grid::TileGrid;
using v3d::grid::hasLineOfSight;
using v3d::grid::sightLine;
using v3d::grid::tileDistance;

namespace {

// the preprocessor splits a macro argument on every comma outside parentheses, so a braced
// TileCoord cannot be written inside a BOOST_CHECK. this is the same literal in a form the
// assertion macros can take
constexpr TileCoord at(int x, int y) {
    return TileCoord{ x, y };
}

TileGrid makeGrid() {
    return TileGrid(9, 9);
}

// whether every consecutive pair in a traced line is one 8 way step apart. a line that
// teleports over a tile is the failure mode a length assertion alone would not catch
bool contiguous(const std::vector<TileCoord>& line) {
    for (std::size_t i = 1; i < line.size(); ++i) {
        if (tileDistance(line[i - 1], line[i]) != 1) {
            return false;
        }
    }
    return true;
}

std::vector<TileCoord> reversed(std::vector<TileCoord> line) {
    std::ranges::reverse(line);
    return line;
}

// every tile of a grid, so a sweep can take every ordered pair of them
std::vector<TileCoord> allTiles(const TileGrid& grid) {
    std::vector<TileCoord> tiles;
    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            tiles.push_back(at(x, y));
        }
    }
    return tiles;
}

// splitmix64's finaliser over the tile and the seed. the sweep below needs a scatter that is
// varied and reproducible, and nothing else - no state has to be carried between tiles
std::uint64_t mix(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

// a grid with roughly a fifth of its tiles walled off, from a seed.
//
// layouts are generated rather than authored because the asymmetry the sweep is hunting for
// only shows up on particular geometry, and a handful of hand drawn maps would be a handful
// of guesses about which geometry that is
TileGrid scatterFullCover(std::uint64_t seed) {
    TileGrid grid = makeGrid();

    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            const std::uint64_t hash = mix(seed ^ (static_cast<std::uint64_t>(y) << 8) ^ static_cast<std::uint64_t>(x));
            if (hash % 5U == 0U) {
                grid.setCover(at(x, y), Cover::Full);
            }
        }
    }
    return grid;
}

std::string describe(TileCoord tile) {
    return "(" + std::to_string(tile.x) + ", " + std::to_string(tile.y) + ")";
}

};  // namespace

BOOST_AUTO_TEST_CASE(lineofsight_open_ground_never_blocks_test) {
    const TileGrid grid = makeGrid();

    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(0, 0), at(8, 8)), true);
    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(4, 0), at(4, 8)), true);
    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(0, 3), at(7, 5)), true);
}

BOOST_AUTO_TEST_CASE(lineofsight_a_tile_sees_itself_and_its_neighbours_test) {
    const TileGrid grid = makeGrid();

    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(4, 4), at(4, 4)), true);
    for (int y = 3; y <= 5; ++y) {
        for (int x = 3; x <= 5; ++x) {
            BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(4, 4), at(x, y)), true);
        }
    }
}

BOOST_AUTO_TEST_CASE(lineofsight_only_full_cover_blocks_test) {
    TileGrid grid = makeGrid();

    grid.setCover(at(4, 4), Cover::Half);
    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(2, 4), at(6, 4)), true);

    grid.setCover(at(4, 4), Cover::Full);
    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(2, 4), at(6, 4)), false);

    // beside the wall rather than through it
    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(2, 5), at(6, 5)), true);
}

BOOST_AUTO_TEST_CASE(lineofsight_the_endpoints_are_not_tested_test) {
    TileGrid grid = makeGrid();
    grid.setCover(at(2, 4), Cover::Full);
    grid.setCover(at(6, 4), Cover::Full);

    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(2, 4), at(6, 4)), true);
    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(6, 4), at(2, 4)), true);
}

BOOST_AUTO_TEST_CASE(lineofsight_a_callers_blocker_adds_to_the_grids_own_test) {
    const TileGrid grid = makeGrid();
    const SightBlocker smoke = [](TileCoord tile) {
        return tile == at(4, 4);
    };

    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(2, 4), at(6, 4)), true);
    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(2, 4), at(6, 4), smoke), false);

    // the endpoint rule covers the caller's blocker too
    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(4, 4), at(6, 4), smoke), true);
}

BOOST_AUTO_TEST_CASE(lineofsight_a_corner_stops_sight_only_when_both_tiles_do_test) {
    TileGrid grid = makeGrid();

    grid.setCover(at(5, 4), Cover::Full);
    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(4, 4), at(6, 6)), true);
    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(4, 4), at(5, 5)), true);

    grid.setCover(at(4, 5), Cover::Full);
    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(4, 4), at(6, 6)), false);
    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(4, 4), at(5, 5)), false);
}

BOOST_AUTO_TEST_CASE(lineofsight_an_off_grid_endpoint_sees_nothing_test) {
    const TileGrid grid = makeGrid();

    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(-1, 4), at(4, 4)), false);
    BOOST_CHECK_EQUAL(hasLineOfSight(grid, at(4, 4), at(9, 4)), false);
}

BOOST_AUTO_TEST_CASE(sightline_starts_and_ends_where_it_is_asked_test) {
    const TileGrid grid = makeGrid();

    const std::vector<TileCoord> line = sightLine(grid, at(1, 2), at(7, 5));
    BOOST_CHECK_EQUAL(line.front() == at(1, 2), true);
    BOOST_CHECK_EQUAL(line.back() == at(7, 5), true);
    BOOST_CHECK_EQUAL(contiguous(line), true);
}

BOOST_AUTO_TEST_CASE(sightline_a_tile_to_itself_is_one_tile_long_test) {
    const TileGrid grid = makeGrid();

    BOOST_CHECK_EQUAL(sightLine(grid, at(4, 4), at(4, 4)) ==
        std::vector<TileCoord>{ at(4, 4) }, true);
}

BOOST_AUTO_TEST_CASE(sightline_an_orthogonal_run_is_the_tiles_between_test) {
    const TileGrid grid = makeGrid();

    const std::vector<TileCoord> expected{ at(2, 4), at(3, 4),
        at(4, 4), at(5, 4) };
    BOOST_CHECK_EQUAL(sightLine(grid, at(2, 4), at(5, 4)) == expected, true);
}

BOOST_AUTO_TEST_CASE(sightline_a_diagonal_squeezes_through_the_corner_test) {
    const TileGrid grid = makeGrid();

    const std::vector<TileCoord> expected{ at(2, 2), at(3, 3), at(4, 4) };
    BOOST_CHECK_EQUAL(sightLine(grid, at(2, 2), at(4, 4)) == expected, true);
}

BOOST_AUTO_TEST_CASE(sightline_an_off_grid_endpoint_traces_nothing_test) {
    const TileGrid grid = makeGrid();

    BOOST_CHECK_EQUAL(sightLine(grid, at(4, 4), at(-1, 0)).empty(), true);
    BOOST_CHECK_EQUAL(sightLine(grid, at(0, 9), at(4, 4)).empty(), true);
}

// the sweeps below are over every ordered pair rather than a sample, because the asymmetry a
// naive trace produces appears only where a line passes exactly through a corner, and nothing
// about a sampled pair says whether it does. one assertion per sweep rather than one per
// pair: 32,000 passing checks say nothing that one does not

BOOST_AUTO_TEST_CASE(lineofsight_is_symmetric_over_every_pair_test) {
    std::string asymmetric;

    for (const std::uint64_t seed : { 1ULL, 7ULL, 99ULL, 12345ULL, 0ULL }) {
        const TileGrid grid = scatterFullCover(seed);
        const std::vector<TileCoord> tiles = allTiles(grid);

        for (const TileCoord a : tiles) {
            for (const TileCoord b : tiles) {
                if (hasLineOfSight(grid, a, b) != hasLineOfSight(grid, b, a) && asymmetric.empty()) {
                    asymmetric = "seed " + std::to_string(seed) + " between " + describe(a) + " and " + describe(b);
                }
            }
        }
    }

    BOOST_CHECK_EQUAL(asymmetric, "");
}

BOOST_AUTO_TEST_CASE(lineofsight_stays_symmetric_with_a_blocker_test) {
    const TileGrid grid = scatterFullCover(4242ULL);

    // an extra blocker with a shape of its own, so the sweep is not just re-testing the
    // grid's cover under another name
    const SightBlocker smoke = [](TileCoord tile) {
        return (tile.x * 3 + tile.y * 5) % 7 == 0;
    };

    std::string asymmetric;
    const std::vector<TileCoord> tiles = allTiles(grid);
    for (const TileCoord a : tiles) {
        for (const TileCoord b : tiles) {
            if (hasLineOfSight(grid, a, b, smoke) != hasLineOfSight(grid, b, a, smoke) && asymmetric.empty()) {
                asymmetric = describe(a) + " and " + describe(b);
            }
        }
    }

    BOOST_CHECK_EQUAL(asymmetric, "");
}

BOOST_AUTO_TEST_CASE(sightline_reverses_exactly_over_every_pair_test) {
    const TileGrid grid = makeGrid();
    const std::vector<TileCoord> tiles = allTiles(grid);

    std::string wrong;
    for (const TileCoord a : tiles) {
        for (const TileCoord b : tiles) {
            const std::vector<TileCoord> forward = sightLine(grid, a, b);
            const bool ok = forward == reversed(sightLine(grid, b, a)) && contiguous(forward) &&
                forward.front() == a && forward.back() == b;
            if (!ok && wrong.empty()) {
                wrong = describe(a) + " to " + describe(b);
            }
        }
    }

    BOOST_CHECK_EQUAL(wrong, "");
}

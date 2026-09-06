/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "LineOfSight.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <vector>

namespace v3d::grid {

namespace {

int signOf(int value) {
    return static_cast<int>(value > 0) - static_cast<int>(value < 0);
}

/**
 * The fixed order the two endpoints are put in before anything is traced.
 *
 * Any total order would do; what matters is that one trace serves both directions, so no tie
 * inside it can be broken one way for one end and the other way for the other. Row before
 * column, to match the grid's own row major layout.
 **/
bool precedes(TileCoord a, TileCoord b) {
    return a.y != b.y ? a.y < b.y : a.x < b.x;
}

/**
 * One tile the traced segment moves onto.
 **/
struct Step {
    TileCoord tile;

    /**
     * Set when the segment passed exactly through the point four tiles share, so it moved
     * diagonally and touched neither tile it passed between. Those two are corner, and they
     * stop sight only together.
     **/
    bool diagonal{false};
    std::pair<TileCoord, TileCoord> corner;
};

/**
 * Walk every tile the segment between two tile centres touches, from exclusive.
 *
 * Tiles are unit squares centred on their coordinate, so the boundaries the segment crosses
 * sit at half integers. After i steps along x and j along y, the segment meets the next x
 * boundary before the next y boundary exactly when ady * (2i + 1) < adx * (2j + 1) - integer
 * arithmetic throughout, so the tie is exact rather than a tolerance, and the tie is the
 * corner case.
 *
 * @param onStep called for each tile entered; returning false stops the walk
 **/
template<typename OnStep>
void trace(TileCoord from, TileCoord to, OnStep onStep) {
    const int sx = signOf(to.x - from.x);
    const int sy = signOf(to.y - from.y);

    const std::int64_t adx = std::abs(to.x - from.x);
    const std::int64_t ady = std::abs(to.y - from.y);

    auto at = [from, sx, sy](std::int64_t i, std::int64_t j) {
        return TileCoord{ from.x + sx * static_cast<int>(i), from.y + sy * static_cast<int>(j) };
    };

    std::int64_t i = 0;
    std::int64_t j = 0;
    while (i != adx || j != ady) {
        const std::int64_t boundary = ady * (2 * i + 1) - adx * (2 * j + 1);

        Step step;
        if (boundary == 0) {
            step.diagonal = true;
            step.corner = { at(i + 1, j), at(i, j + 1) };
            ++i;
            ++j;
        } else if (boundary < 0) {
            ++i;
        } else {
            ++j;
        }

        step.tile = at(i, j);
        if (!onStep(step)) {
            return;
        }
    }
}

};  // namespace

bool hasLineOfSight(const TileGrid& grid, TileCoord from, TileCoord to, const SightBlocker& blocks) {
    if (!grid.contains(from) || !grid.contains(to)) {
        return false;
    }

    // one trace for both directions: whichever way the caller asked, the same line is
    // walked, so the answer cannot depend on which end is asking
    if (precedes(to, from)) {
        std::swap(from, to);
    }

    const auto blocked = [&grid, &blocks](TileCoord tile) {
        return grid.cover(tile) == Cover::Full || (blocks && blocks(tile));
    };

    bool clear = true;
    trace(from, to, [&](const Step& step) {
        // the pair a diagonal squeezes between stops sight only together; the tile it lands
        // on is tested like any other, and the far endpoint is not tested at all
        if (step.diagonal && blocked(step.corner.first) && blocked(step.corner.second)) {
            clear = false;
        }
        if (step.tile != to && blocked(step.tile)) {
            clear = false;
        }
        return clear;
    });

    return clear;
}

std::vector<TileCoord> sightLine(const TileGrid& grid, TileCoord from, TileCoord to) {
    if (!grid.contains(from) || !grid.contains(to)) {
        return {};
    }

    const bool reversed = precedes(to, from);
    if (reversed) {
        std::swap(from, to);
    }

    std::vector<TileCoord> line{ from };
    trace(from, to, [&line](const Step& step) {
        line.push_back(step.tile);
        return true;
    });

    if (reversed) {
        std::ranges::reverse(line);
    }

    return line;
}

};  // namespace v3d::grid

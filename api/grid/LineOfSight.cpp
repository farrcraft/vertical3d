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
bool precedes(TileCoord first, TileCoord second) {
    return first.y != second.y ? first.y < second.y : first.x < second.x;
}

/**
 * One tile the traced segment moves onto.
 **/
struct Step {
    TileCoord tile;

    /**
     * Set when the segment passed exactly through the point four tiles share, so it moved
     * diagonally and touched neither tile it passed between. Those two tiles block sight
     * only when both of them block.
     **/
    bool diagonal{false};
    std::pair<TileCoord, TileCoord> corner;
};

/**
 * Walk every tile the segment between two tile centres touches, from exclusive.
 *
 * Tiles are unit squares centred on their coordinate, so the boundaries the segment crosses
 * sit at half integers. After xSteps steps along x and ySteps along y, the segment meets the
 * next x boundary before the next y boundary exactly when
 * ady * (2 * xSteps + 1) < adx * (2 * ySteps + 1) - integer arithmetic throughout, so the tie
 * is exact rather than a tolerance, and the tie is the corner case.
 *
 * @param onStep called for each tile entered; returning false stops the walk
 **/
template<typename OnStep>
void trace(TileCoord from, TileCoord to, OnStep onStep) {
    const int sx = signOf(to.x - from.x);
    const int sy = signOf(to.y - from.y);

    const std::int64_t adx = std::abs(to.x - from.x);
    const std::int64_t ady = std::abs(to.y - from.y);

    auto at = [from, sx, sy](std::int64_t alongX, std::int64_t alongY) {
        return TileCoord{ from.x + (sx * static_cast<int>(alongX)), from.y + (sy * static_cast<int>(alongY)) };
    };

    std::int64_t xSteps = 0;
    std::int64_t ySteps = 0;
    while (xSteps != adx || ySteps != ady) {
        const std::int64_t boundary = (ady * ((2 * xSteps) + 1)) - (adx * ((2 * ySteps) + 1));

        Step step;
        if (boundary == 0) {
            step.diagonal = true;
            step.corner = { at(xSteps + 1, ySteps), at(xSteps, ySteps + 1) };
            ++xSteps;
            ++ySteps;
        } else if (boundary < 0) {
            ++xSteps;
        } else {
            ++ySteps;
        }

        step.tile = at(xSteps, ySteps);
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

    // one trace for both directions: whatever the argument order, the same line is traced,
    // so the result does not depend on which endpoint comes first
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

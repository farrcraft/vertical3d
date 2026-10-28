/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Pathfinding.h"

#include "DistanceField.h"
#include "GridSet.h"
#include "ReachableTile.h"
#include "TileCoord.h"
#include "TileFilter.h"
#include "TileGrid.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

namespace v3d::grid {

namespace {

constexpr int UNREACHED = std::numeric_limits<int>::max();

/**
 * Neighbour offsets, orthogonals first. The order breaks ties between routes of equal cost,
 * so it is fixed: the same query returns the same path on every run.
 **/
constexpr std::array<TileCoord, 8> STEPS{
    TileCoord{ 1, 0 }, TileCoord{ -1, 0 }, TileCoord{ 0, 1 }, TileCoord{ 0, -1 },
    TileCoord{ 1, 1 }, TileCoord{ 1, -1 }, TileCoord{ -1, 1 }, TileCoord{ -1, -1 }
};

int stepCost(TileCoord step) {
    return step.x != 0 && step.y != 0 ? DIAGONAL_STEP_COST : ORTHOGONAL_STEP_COST;
}

/**
 * One tile in the priority queue, ordered by estimated total cost. flood() uses the same type
 * with a heuristic of zero, which makes it Dijkstra's algorithm.
 **/
struct Frontier {
    int estimate{0};
    std::size_t index{0};

    friend bool operator>(const Frontier& lhs, const Frontier& rhs) {
        return lhs.estimate > rhs.estimate;
    }
};

typedef std::priority_queue<Frontier, std::vector<Frontier>, std::greater<>> Queue;

/**
 * The grids plus the caller's filter, and the movement rules that read both.
 **/
class Walk {
 public:
    Walk(const GridSet& grids, const TileFilter& enterable) :
        grids_(grids),
        enterable_(enterable) {
    }

    std::size_t count() const {
        return grids_.count();
    }

    std::size_t index(TileCoord tile) const {
        return grids_.index(tile);
    }

    TileCoord coord(std::size_t index) const {
        return grids_.tile(index);
    }

    /**
     * Whether a tile may be entered at all. A tile in no grid is impassable, so this needs no
     * separate bounds test.
     **/
    bool open(TileCoord tile) const {
        return grids_.passable(tile) && (!enterable_ || enterable_(tile));
    }

    /**
     * Whether a step may be taken from a tile.
     *
     * A diagonal additionally requires one of the two tiles it passes between to be open,
     * so a wall laid corner to corner cannot be squeezed through. One blocked corner still
     * allows the step, so a mover can round the end of a wall.
     **/
    bool allowed(TileCoord from, TileCoord step) const {
        if (!open(TileCoord{ from.x + step.x, from.y + step.y })) {
            return false;
        }
        if (step.x == 0 || step.y == 0) {
            return true;
        }
        return open(TileCoord{ from.x + step.x, from.y }) || open(TileCoord{ from.x, from.y + step.y });
    }

 private:
    const GridSet& grids_;
    const TileFilter& enterable_;
};

/**
 * Finds every tile reachable from seed, and the cost of reaching it. This is Dijkstra's
 * algorithm: the A* search of findPath() with a heuristic of zero. A step that would take the
 * cost past budget is not taken.
 *
 * @param settled called once for each tile when its cost is final, cheapest first. May be
 *        empty
 * @return each tile's cost by grid index, or UNREACHED where the flood did not reach
 **/
std::vector<int> flood(const Walk& walk, TileCoord seed, int budget, const std::function<void(TileCoord, int)>& settled) {
    std::vector<int> best(walk.count(), UNREACHED);
    const std::size_t seedIndex = walk.index(seed);
    best[seedIndex] = 0;

    Queue frontier;
    frontier.push(Frontier{ 0, seedIndex });

    while (!frontier.empty()) {
        const Frontier current = frontier.top();
        frontier.pop();

        // stale: this tile was queued again more cheaply after this entry was pushed
        if (current.estimate > best[current.index]) {
            continue;
        }

        const TileCoord tile = walk.coord(current.index);
        if (settled) {
            settled(tile, current.estimate);
        }

        for (const TileCoord step : STEPS) {
            if (!walk.allowed(tile, step)) {
                continue;
            }
            const int cost = current.estimate + stepCost(step);
            if (cost > budget) {
                continue;
            }
            const std::size_t nextIndex = walk.index(TileCoord{ tile.x + step.x, tile.y + step.y });
            if (cost >= best[nextIndex]) {
                continue;
            }
            best[nextIndex] = cost;
            frontier.push(Frontier{ cost, nextIndex });
        }
    }
    return best;
}

};  // namespace

int tileDistance(TileCoord from, TileCoord to) {
    return std::max(std::abs(from.x - to.x), std::abs(from.y - to.y));
}

std::vector<TileCoord> findPath(const TileGrid& grid, TileCoord start, TileCoord goal, const TileFilter& enterable) {
    return findPath(GridSet::local(grid), start, goal, enterable);
}

std::vector<TileCoord> findPath(const GridSet& grids, TileCoord start, TileCoord goal, const TileFilter& enterable) {
    if (!grids.contains(start) || !grids.contains(goal)) {
        return {};
    }
    if (start == goal) {
        return { start };
    }

    const Walk walk(grids, enterable);
    if (!walk.open(goal)) {
        return {};
    }

    std::vector<int> best(walk.count(), UNREACHED);
    std::vector<std::size_t> from(walk.count(), walk.count());

    const std::size_t startIndex = walk.index(start);
    best[startIndex] = 0;

    Queue frontier;
    frontier.push(Frontier{ tileDistance(start, goal), startIndex });

    while (!frontier.empty()) {
        const Frontier current = frontier.top();
        frontier.pop();

        const TileCoord tile = walk.coord(current.index);
        const int reached = best[current.index];

        // stale: this tile was queued again more cheaply after this entry was pushed
        if (current.estimate > reached + tileDistance(tile, goal)) {
            continue;
        }

        if (tile == goal) {
            std::vector<TileCoord> path;
            for (std::size_t step = current.index; step != walk.count(); step = from[step]) {
                path.push_back(walk.coord(step));
            }
            std::ranges::reverse(path);
            return path;
        }

        for (const TileCoord step : STEPS) {
            if (!walk.allowed(tile, step)) {
                continue;
            }
            const TileCoord next{ tile.x + step.x, tile.y + step.y };
            const std::size_t nextIndex = walk.index(next);
            const int cost = reached + stepCost(step);
            if (cost >= best[nextIndex]) {
                continue;
            }
            best[nextIndex] = cost;
            from[nextIndex] = current.index;
            frontier.push(Frontier{ cost + tileDistance(next, goal), nextIndex });
        }
    }

    return {};
}

std::vector<ReachableTile> reachableTiles(const TileGrid& grid, TileCoord start, int budget,
    const TileFilter& enterable) {
    return reachableTiles(GridSet::local(grid), start, budget, enterable);
}

std::vector<ReachableTile> reachableTiles(const GridSet& grids, TileCoord start, int budget,
    const TileFilter& enterable) {
    if (budget < 0) {
        throw std::invalid_argument("a movement budget cannot be " + std::to_string(budget));
    }
    if (!grids.contains(start)) {
        return {};
    }

    std::vector<ReachableTile> reached;
    flood(Walk(grids, enterable), start, budget,
        [&reached](TileCoord tile, int cost) { reached.push_back(ReachableTile{ tile, cost }); });
    return reached;
}

DistanceField::DistanceField(const TileGrid& grid, TileCoord goal, const TileFilter& enterable) :
    DistanceField(GridSet::local(grid), goal, enterable) {
}

DistanceField::DistanceField(const GridSet& grids, TileCoord goal, const TileFilter& enterable) :
    grids_(grids) {
    if (!grids.contains(goal)) {
        return;
    }

    // the flood goes outward from the goal and records each cost on the tile a step leads to.
    // That is also the cost of the route back, because a step costs the same in both
    // directions, and a diagonal passes between the same two tiles from either end. A budget
    // of UNREACHED sets no limit. UNREACHED and UNREACHABLE are the same value
    costs_ = flood(Walk(grids, enterable), goal, UNREACHED, {});
}

int DistanceField::cost(TileCoord tile) const {
    if (costs_.empty() || !grids_.contains(tile)) {
        return UNREACHABLE;
    }
    return costs_[grids_.index(tile)];
}

bool DistanceField::reaches(TileCoord tile) const {
    return cost(tile) != UNREACHABLE;
}

};  // namespace v3d::grid

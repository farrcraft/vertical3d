/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Pathfinding.h"

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
 * Neighbour offsets, orthogonals first. The order is what breaks ties between routes of
 * equal cost, so it is fixed rather than incidental: the same query returns the same path on
 * every run, which is what lets a result be reproduced.
 **/
constexpr std::array<TileCoord, 8> STEPS{
    TileCoord{ 1, 0 }, TileCoord{ -1, 0 }, TileCoord{ 0, 1 }, TileCoord{ 0, -1 },
    TileCoord{ 1, 1 }, TileCoord{ 1, -1 }, TileCoord{ -1, 1 }, TileCoord{ -1, -1 }
};

int stepCost(TileCoord step) {
    return step.x != 0 && step.y != 0 ? DIAGONAL_STEP_COST : ORTHOGONAL_STEP_COST;
}

/**
 * One tile in the priority queue, ordered by total estimated cost. Dijkstra is the same type
 * with the heuristic left at zero.
 **/
struct Frontier {
    int estimate{0};
    std::size_t index{0};

    friend bool operator>(const Frontier& a, const Frontier& b) {
        return a.estimate > b.estimate;
    }
};

typedef std::priority_queue<Frontier, std::vector<Frontier>, std::greater<>> Queue;

/**
 * The grid plus the caller's filter, and the movement rules that read both.
 **/
class Walk {
 public:
    Walk(const TileGrid& grid, const TileFilter& enterable) :
        grid_(grid),
        enterable_(enterable) {
    }

    std::size_t count() const {
        return static_cast<std::size_t>(grid_.width()) * static_cast<std::size_t>(grid_.height());
    }

    std::size_t index(TileCoord tile) const {
        return static_cast<std::size_t>(tile.y) * static_cast<std::size_t>(grid_.width()) +
            static_cast<std::size_t>(tile.x);
    }

    TileCoord coord(std::size_t index) const {
        const int width = grid_.width();
        return TileCoord{ static_cast<int>(index) % width, static_cast<int>(index) / width };
    }

    /**
     * Whether a tile may be entered at all. Off grid tiles are impassable, so this needs no
     * separate bounds test.
     **/
    bool open(TileCoord tile) const {
        return grid_.passable(tile) && (!enterable_ || enterable_(tile));
    }

    /**
     * Whether a step may be taken from a tile.
     *
     * A diagonal additionally requires one of the two tiles it passes between to be open,
     * so a wall laid corner to corner cannot be squeezed through. One blocked corner still
     * admits the step: rounding the end of a wall passes through nothing.
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
    const TileGrid& grid_;
    const TileFilter& enterable_;
};

};  // namespace

int tileDistance(TileCoord a, TileCoord b) {
    return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y));
}

std::vector<TileCoord> findPath(const TileGrid& grid, TileCoord start, TileCoord goal, const TileFilter& enterable) {
    if (!grid.contains(start) || !grid.contains(goal)) {
        return {};
    }
    if (start == goal) {
        return { start };
    }

    const Walk walk(grid, enterable);
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
    if (budget < 0) {
        throw std::invalid_argument("a movement budget cannot be " + std::to_string(budget));
    }
    if (!grid.contains(start)) {
        return {};
    }

    const Walk walk(grid, enterable);

    std::vector<int> best(walk.count(), UNREACHED);

    const std::size_t startIndex = walk.index(start);
    best[startIndex] = 0;

    Queue frontier;
    frontier.push(Frontier{ 0, startIndex });

    std::vector<ReachableTile> reached;

    while (!frontier.empty()) {
        const Frontier current = frontier.top();
        frontier.pop();

        if (current.estimate > best[current.index]) {
            continue;
        }

        const TileCoord tile = walk.coord(current.index);
        reached.push_back(ReachableTile{ tile, current.estimate });

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

    return reached;
}

DistanceField::DistanceField(const TileGrid& grid, TileCoord goal, const TileFilter& enterable) {
    if (!grid.contains(goal)) {
        return;
    }

    const Walk walk(grid, enterable);

    width_ = grid.width();
    costs_.assign(walk.count(), UNREACHABLE);

    const std::size_t goalIndex = walk.index(goal);
    costs_[goalIndex] = 0;

    Queue frontier;
    frontier.push(Frontier{ 0, goalIndex });

    while (!frontier.empty()) {
        const Frontier current = frontier.top();
        frontier.pop();

        if (current.estimate > costs_[current.index]) {
            continue;
        }

        // the step is recorded on the tile it leads *to*, and walked outward from the goal.
        // it reads as the cost of coming back the other way because both halves of the rule
        // are symmetric: a diagonal costs the same either way, and the two corners it
        // squeezes between are the same two tiles from both ends.
        const TileCoord tile = walk.coord(current.index);
        for (const TileCoord step : STEPS) {
            if (!walk.allowed(tile, step)) {
                continue;
            }
            const std::size_t nextIndex = walk.index(TileCoord{ tile.x + step.x, tile.y + step.y });
            const int cost = current.estimate + stepCost(step);
            if (cost >= costs_[nextIndex]) {
                continue;
            }
            costs_[nextIndex] = cost;
            frontier.push(Frontier{ cost, nextIndex });
        }
    }
}

int DistanceField::cost(TileCoord tile) const {
    if (width_ <= 0 || tile.x < 0 || tile.x >= width_ || tile.y < 0) {
        return UNREACHABLE;
    }

    const std::size_t index =
        static_cast<std::size_t>(tile.y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(tile.x);

    return index < costs_.size() ? costs_[index] : UNREACHABLE;
}

bool DistanceField::reaches(TileCoord tile) const {
    return cost(tile) != UNREACHABLE;
}

};  // namespace v3d::grid

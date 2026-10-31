/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <functional>
#include <queue>
#include <vector>

namespace v3d::grid::internal {

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

};  // namespace v3d::grid::internal

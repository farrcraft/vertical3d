/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Hierarchy.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::trace {

namespace {

/** How many bins the centres along the split axis are sorted into. **/
constexpr std::size_t BINS = 16;

/** A node of this many items or fewer is a leaf. **/
constexpr std::uint32_t SMALLEST_SPLIT = 2;

/** A node of this many items or fewer is a leaf when splitting it would cost more. **/
constexpr std::uint32_t LARGEST_LEAF = 8;

/** How much a box grows, as a fraction of the largest coordinate it holds. **/
constexpr float GROWTH = 1.0e-5F;

/** Which bin a centre falls in along an axis of the centres' range. **/
std::size_t bin(float centre, float low, float scale) {
    const float at = (centre - low) * scale;
    return std::min(BINS - 1, static_cast<std::size_t>(std::max(0.0F, at)));
}

};  // namespace

void Hierarchy::Box::extend(const glm::vec3 & point) {
    min = glm::min(min, point);
    max = glm::max(max, point);
}

void Hierarchy::Box::extend(const Box & other) {
    min = glm::min(min, other.min);
    max = glm::max(max, other.max);
}

bool Hierarchy::Box::empty() const {
    return min.x > max.x || min.y > max.y || min.z > max.z;
}

float Hierarchy::Box::area() const {
    if (empty()) {
        return 0.0F;
    }
    const glm::vec3 size = max - min;
    return (size.x * size.y) + (size.y * size.z) + (size.z * size.x);
}

glm::vec3 Hierarchy::Box::centre() const {
    return 0.5F * (min + max);
}

void Hierarchy::build(const std::vector<Box> & boxes) {
    nodes_.clear();
    order_.clear();
    if (boxes.empty()) {
        return;
    }
    std::vector<Box> grown(boxes);
    std::vector<glm::vec3> centres(boxes.size());
    for (std::size_t i = 0; i < grown.size(); i++) {
        Box & box = grown[i];
        const glm::vec3 largest = glm::max(glm::abs(box.min), glm::abs(box.max));
        const float pad = GROWTH * (1.0F + std::max(largest.x, std::max(largest.y, largest.z)));
        box.min -= glm::vec3(pad);
        box.max += glm::vec3(pad);
        centres[i] = box.centre();
        order_.push_back(static_cast<std::uint32_t>(i));
    }
    nodes_.reserve((2 * boxes.size()) - 1);
    Node root;
    root.first = 0;
    root.count = static_cast<std::uint32_t>(boxes.size());
    nodes_.push_back(root);
    split(0, grown, centres, 0);
}

bool Hierarchy::empty() const {
    return nodes_.empty();
}

std::size_t Hierarchy::descend(std::uint32_t node, const Probe & probe, std::uint32_t* stack, float* entries,
    std::size_t depth) const {
    const std::uint32_t one = node + 1;
    const std::uint32_t other = nodes_[node].second;
    float oneEntry = 0.0F;
    float otherEntry = 0.0F;
    const bool oneHit = enters(nodes_[one].box, probe, &oneEntry);
    const bool otherHit = enters(nodes_[other].box, probe, &otherEntry);
    // the nearer child goes on the stack last, so it is visited first
    const bool swapped = oneHit && otherHit && otherEntry < oneEntry;
    const std::uint32_t first = swapped ? other : one;
    const std::uint32_t last = swapped ? one : other;
    const bool firstHit = swapped ? otherHit : oneHit;
    const bool lastHit = swapped ? oneHit : otherHit;
    if (lastHit) {
        stack[depth] = last;
        entries[depth] = swapped ? oneEntry : otherEntry;
        depth++;
    }
    if (firstHit) {
        stack[depth] = first;
        entries[depth] = swapped ? otherEntry : oneEntry;
        depth++;
    }
    return depth;
}

void Hierarchy::split(std::uint32_t node, const std::vector<Box> & boxes, const std::vector<glm::vec3> & centres,
    unsigned int depth) {
    const std::uint32_t first = nodes_[node].first;
    const std::uint32_t count = nodes_[node].count;
    Box bounds;
    Box spread;
    for (std::uint32_t k = first; k < first + count; k++) {
        bounds.extend(boxes[order_[k]]);
        spread.extend(centres[order_[k]]);
    }
    nodes_[node].box = bounds;
    if (count <= SMALLEST_SPLIT || depth >= DEEPEST) {
        return;
    }

    // the axis the centres spread furthest along. Items whose centres all coincide cannot be
    // told apart by any split, so they stay one leaf
    const glm::vec3 extent = spread.max - spread.min;
    int axis = 0;
    if (extent.y > extent[axis]) {
        axis = 1;
    }
    if (extent.z > extent[axis]) {
        axis = 2;
    }
    if (!(extent[axis] > 0.0F)) {
        return;
    }
    const float low = spread.min[axis];
    const float scale = static_cast<float>(BINS) / extent[axis];

    std::array<Box, BINS> binned;
    std::array<std::uint32_t, BINS> counts{};
    for (std::uint32_t k = first; k < first + count; k++) {
        const std::size_t at = bin(centres[order_[k]][axis], low, scale);
        binned[at].extend(boxes[order_[k]]);
        counts[at]++;
    }

    // the cost of each split, with the bins below it on one side: the area of each side times
    // the items on it, over the area of the whole
    std::array<float, BINS> below{};
    std::array<std::uint32_t, BINS> belowCounts{};
    Box running;
    std::uint32_t runningCount = 0;
    for (std::size_t at = 0; at + 1 < BINS; at++) {
        running.extend(binned[at]);
        runningCount += counts[at];
        below[at] = running.area() * static_cast<float>(runningCount);
        belowCounts[at] = runningCount;
    }
    float best = std::numeric_limits<float>::max();
    std::size_t bestSplit = 0;
    running = Box();
    runningCount = 0;
    for (std::size_t at = BINS - 1; at > 0; at--) {
        running.extend(binned[at]);
        runningCount += counts[at];
        if (belowCounts[at - 1] == 0 || runningCount == 0) {
            continue;
        }
        const float cost = below[at - 1] + (running.area() * static_cast<float>(runningCount));
        if (cost < best) {
            best = cost;
            bestSplit = at;
        }
    }
    const float whole = bounds.area();
    const float splitCost = whole > 0.0F ? 1.0F + (best / whole) : static_cast<float>(count);
    if (bestSplit == 0 || (splitCost >= static_cast<float>(count) && count <= LARGEST_LEAF)) {
        return;
    }

    std::uint32_t* const start = order_.data() + first;
    std::uint32_t* const middle = std::partition(start, start + count, [&](std::uint32_t item) {
        return bin(centres[item][axis], low, scale) < bestSplit;
    });
    const std::uint32_t lower = static_cast<std::uint32_t>(middle - start);

    Node left;
    left.first = first;
    left.count = lower;
    const std::uint32_t leftIndex = static_cast<std::uint32_t>(nodes_.size());
    nodes_.push_back(left);
    split(leftIndex, boxes, centres, depth + 1);

    Node right;
    right.first = first + lower;
    right.count = count - lower;
    const std::uint32_t rightIndex = static_cast<std::uint32_t>(nodes_.size());
    nodes_.push_back(right);
    split(rightIndex, boxes, centres, depth + 1);

    nodes_[node].count = 0;
    nodes_[node].second = rightIndex;
}

bool Hierarchy::enters(const Box & box, const Probe & probe, float* entry) {
    const glm::vec3 & origin = probe.origin;
    const glm::vec3 & direction = probe.direction;
    const glm::vec3 & inverse = probe.inverse;
    float in = probe.from;
    float out = std::numeric_limits<float>::max();
    for (int axis = 0; axis < 3; axis++) {
        if (direction[axis] == 0.0F) {
            // parallel to this pair of slabs, so the origin is between them or the ray misses
            if (origin[axis] < box.min[axis] || origin[axis] > box.max[axis]) {
                return false;
            }
            continue;
        }
        float enter = (box.min[axis] - origin[axis]) * inverse[axis];
        float leave = (box.max[axis] - origin[axis]) * inverse[axis];
        if (enter > leave) {
            std::swap(enter, leave);
        }
        in = std::max(in, enter);
        out = std::min(out, leave);
        if (in > out) {
            return false;
        }
    }
    *entry = in;
    return true;
}

};  // namespace v3d::render::offline::trace

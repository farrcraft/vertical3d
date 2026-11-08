/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/internal/Binary.h>

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Builder.h>
#include <moya/libmoya/device/CpuDevice.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::moya::accel::internal {

BinaryBuilder::BinaryBuilder(device::CpuDevice & device, const Clip & clip, const Options & options, float rootArea) :
    device_(device), clip_(clip), options_(options), rootArea_(rootArea) {
}

Binary BinaryBuilder::build(std::vector<Reference> references, const Box & box, std::size_t budget) const {
    Binary out;
    node(std::move(references), box, budget, 0, out);
    return out;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the budget, then how deep the node lies
std::uint32_t BinaryBuilder::node(std::vector<Reference> references, const Box & box, std::size_t budget, int depth, Binary & out) const {
    const std::size_t count = references.size();
    if (count <= 1 || depth >= kDeepest) {
        return leaf(references, box, out);
    }

    Split best;
    best.cost = std::numeric_limits<float>::infinity();
    const bool found = objectSplit(references, best);
    if (options_.spatial && budget > 0 && rootArea_ > 0.0F) {
        // a split by centroids whose halves hardly overlap leaves nothing for a spatial split
        const float overlap = found ? best.left.within(best.right).area() : box.area();
        if (overlap / rootArea_ > kOverlap) {
            spatialSplit(references, box, budget, best);
        }
    }

    // costs are in tests of one primitive, a step costing as much as one test
    const float area = box.area();
    const float leafCost = static_cast<float>(count);
    const float splitCost = area > 0.0F ? 1.0F + (best.cost / area) : std::numeric_limits<float>::infinity();
    const bool split = std::isfinite(best.cost);
    if (count <= kLeafMost && (!split || leafCost <= splitCost)) {
        return leaf(references, box, out);
    }

    std::vector<Reference> left;
    std::vector<Reference> right;
    if (split) {
        partition(references, box, best, left, right);
    }
    if (left.empty() || right.empty()) {
        // no split separates them, as when every centroid is at one place: halve them in order
        left.assign(references.begin(), references.begin() + static_cast<std::ptrdiff_t>(count / 2));
        right.assign(references.begin() + static_cast<std::ptrdiff_t>(count / 2), references.end());
    }
    references.clear();
    references.shrink_to_fit();

    Box leftBox;
    Box rightBox;
    for (const Reference & reference : left) {
        leftBox.grow(reference.box);
    }
    for (const Reference & reference : right) {
        rightBox.grow(reference.box);
    }

    // the budget left after this split is shared between the halves by their counts
    const std::size_t added = left.size() + right.size() > count ? left.size() + right.size() - count : 0;
    const std::size_t remaining = budget > added ? budget - added : 0;
    const std::size_t leftBudget = (remaining * left.size()) / (left.size() + right.size());
    const std::size_t rightBudget = remaining - leftBudget;

    const auto at = static_cast<std::uint32_t>(out.nodes.size());
    out.nodes.push_back(BinaryNode { .box = box });
    std::uint32_t leftIndex = 0;
    std::uint32_t rightIndex = 0;
    if (count >= kParallel) {
        std::array<Binary, 2> parts;
        std::array<std::vector<Reference>, 2> halves { std::move(left), std::move(right) };
        const std::array<Box, 2> boxes { leftBox, rightBox };
        const std::array<std::size_t, 2> budgets { leftBudget, rightBudget };
        device_.each(2, [&](std::size_t half) {
            node(std::move(halves[half]), boxes[half], budgets[half], depth + 1, parts[half]);
        });
        leftIndex = join(parts[0], out);
        rightIndex = join(parts[1], out);
    } else {
        leftIndex = node(std::move(left), leftBox, leftBudget, depth + 1, out);
        rightIndex = node(std::move(right), rightBox, rightBudget, depth + 1, out);
    }
    out.nodes[at].left = leftIndex;
    out.nodes[at].right = rightIndex;
    return at;
}

bool BinaryBuilder::objectSplit(const std::vector<Reference> & references, Split & best) {
    Box centroids;
    for (const Reference & reference : references) {
        centroids.grow(reference.box.centre());
    }
    bool found = false;
    for (int axis = 0; axis < 3; ++axis) {
        const float lower = centroids.lower[axis];
        const float extent = centroids.upper[axis] - lower;
        if (!(extent > 0.0F)) {
            continue;
        }
        Bins bins;
        const float scale = static_cast<float>(kBins) / extent;
        for (const Reference & reference : references) {
            const auto bin = std::min(static_cast<std::uint32_t>((reference.box.centre()[axis] - lower) * scale), kBins - 1);
            bins.boxes[bin].grow(reference.box);
            ++bins.entering[bin];
            ++bins.leaving[bin];
        }
        const Split candidate { .spatial = false, .axis = axis, .lower = lower, .scale = scale };
        found = sweep(bins, references.size(), 0, candidate, best) || found;
    }
    return found;
}

bool BinaryBuilder::spatialSplit(const std::vector<Reference> & references, const Box & box, std::size_t budget, Split & best) const {
    bool found = false;
    for (int axis = 0; axis < 3; ++axis) {
        if (!(box.upper[axis] - box.lower[axis] > 0.0F)) {
            continue;
        }
        const Split candidate { .spatial = true, .axis = axis };
        found = sweep(spatialBins(references, box, axis), references.size(), budget, candidate, best) || found;
    }
    return found;
}

BinaryBuilder::Bins BinaryBuilder::spatialBins(const std::vector<Reference> & references, const Box & box, int axis) const {
    const float lower = box.lower[axis];
    const float width = (box.upper[axis] - lower) / static_cast<float>(kBins);
    const auto binOf = [&](float at) {
        const float scaled = (at - lower) / width;
        return std::min(static_cast<std::uint32_t>(std::max(scaled, 0.0F)), kBins - 1);  // checked: in the node's box, so 0 to kBins
    };
    Bins bins;
    for (const Reference & reference : references) {
        const std::uint32_t first = binOf(reference.box.lower[axis]);
        const std::uint32_t last = binOf(reference.box.upper[axis]);
        ++bins.entering[first];
        ++bins.leaving[last];
        if (first == last) {
            bins.boxes[first].grow(reference.box);
            continue;
        }
        for (std::uint32_t bin = first; bin <= last; ++bin) {
            Box slab = box;
            slab.lower[axis] = lower + (static_cast<float>(bin) * width);
            slab.upper[axis] = bin + 1 == kBins ? box.upper[axis] : lower + (static_cast<float>(bin + 1) * width);
            bins.boxes[bin].grow(clip_(reference.primitive, reference.box.within(slab)));
        }
    }
    return bins;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the references held, then those it may add
bool BinaryBuilder::sweep(const Bins & bins, std::size_t total, std::size_t budget, const Split & candidate, Split & best) {
    // the area and count right of each bin's far edge
    std::array<float, kBins> rightArea {};
    std::array<std::size_t, kBins> rightCount {};
    Box right;
    std::size_t count = 0;
    for (std::uint32_t bin = kBins - 1; bin > 0; --bin) {
        right.grow(bins.boxes[bin]);
        count += bins.leaving[bin];
        rightArea[bin - 1] = right.area();
        rightCount[bin - 1] = count;
    }
    bool found = false;
    Box left;
    std::size_t leftCount = 0;
    for (std::uint32_t bin = 0; bin + 1 < kBins; ++bin) {
        left.grow(bins.boxes[bin]);
        leftCount += bins.entering[bin];
        const std::size_t added = leftCount + rightCount[bin] > total ? leftCount + rightCount[bin] - total : 0;
        if (leftCount == 0 || rightCount[bin] == 0 || added > budget) {
            continue;
        }
        const float cost = (left.area() * static_cast<float>(leftCount)) + (rightArea[bin] * static_cast<float>(rightCount[bin]));
        if (cost < best.cost) {
            best = candidate;
            best.bin = bin;
            best.cost = cost;
            best.left = left;
            best.right = Box {};
            for (std::uint32_t other = bin + 1; other < kBins; ++other) {
                best.right.grow(bins.boxes[other]);
            }
            best.added = added;
            found = true;
        }
    }
    return found;
}

void BinaryBuilder::partition(std::vector<Reference> & references, const Box & box, const Split & split, std::vector<Reference> & left,
    std::vector<Reference> & right) const {
    const int axis = split.axis;
    if (!split.spatial) {
        // the centroids' bins, as objectSplit() counted them
        for (const Reference & reference : references) {
            const auto bin = std::min(static_cast<std::uint32_t>((reference.box.centre()[axis] - split.lower) * split.scale), kBins - 1);
            (bin <= split.bin ? left : right).push_back(reference);
        }
        return;
    }
    const float width = (box.upper[axis] - box.lower[axis]) / static_cast<float>(kBins);
    const float plane = box.lower[axis] + (static_cast<float>(split.bin + 1) * width);
    Box leftHalf = box;
    leftHalf.upper[axis] = plane;
    Box rightHalf = box;
    rightHalf.lower[axis] = plane;
    for (const Reference & reference : references) {
        if (reference.box.upper[axis] <= plane) {
            left.push_back(reference);
        } else if (reference.box.lower[axis] >= plane) {
            right.push_back(reference);
        } else {
            const Box leftPart = clip_(reference.primitive, reference.box.within(leftHalf));
            const Box rightPart = clip_(reference.primitive, reference.box.within(rightHalf));
            if (leftPart.empty() && rightPart.empty()) {
                // rounding lost both parts, so the whole goes to one side
                left.push_back(reference);
                continue;
            }
            if (!leftPart.empty()) {
                left.push_back(Reference { .box = leftPart, .primitive = reference.primitive });
            }
            if (!rightPart.empty()) {
                right.push_back(Reference { .box = rightPart, .primitive = reference.primitive });
            }
        }
    }
}

std::uint32_t BinaryBuilder::leaf(const std::vector<Reference> & references, const Box & box, Binary & out) {
    const auto at = static_cast<std::uint32_t>(out.nodes.size());
    BinaryNode node { .box = box, .leaf = true };
    node.first = static_cast<std::uint32_t>(out.references.size());
    node.count = static_cast<std::uint32_t>(references.size());
    for (const Reference & reference : references) {
        out.references.push_back(reference.primitive);
    }
    out.nodes.push_back(node);
    return at;
}

std::uint32_t BinaryBuilder::join(const Binary & part, Binary & out) {
    const auto nodeOffset = static_cast<std::uint32_t>(out.nodes.size());
    const auto referenceOffset = static_cast<std::uint32_t>(out.references.size());
    for (BinaryNode node : part.nodes) {
        if (node.leaf) {
            node.first += referenceOffset;
        } else {
            node.left += nodeOffset;
            node.right += nodeOffset;
        }
        out.nodes.push_back(node);
    }
    out.references.insert(out.references.end(), part.references.begin(), part.references.end());
    return nodeOffset;
}

};  // namespace v3d::moya::accel::internal

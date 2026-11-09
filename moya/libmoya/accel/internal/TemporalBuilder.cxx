/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/internal/TemporalBuilder.h>

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Options.h>
#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/accel/internal/MovingBinary.h>
#include <moya/libmoya/accel/internal/MovingBinaryNode.h>
#include <moya/libmoya/accel/internal/MovingReference.h>
#include <moya/libmoya/device/CpuDevice.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace v3d::moya::accel::internal {

namespace {

LinearBox boundsOf(const std::vector<MovingReference> & references) {
    LinearBox out;
    for (const MovingReference & reference : references) {
        out.grow(reference.box);
    }
    return out;
}

};  // namespace

TemporalBuilder::TemporalBuilder(device::CpuDevice & device, const Sweep & sweep, const Options & options) :
    device_(device), sweep_(sweep), options_(options) {
}

MovingBinary TemporalBuilder::build(std::vector<MovingReference> references, float begin, float end) const {
    MovingBinary out;
    node(std::move(references), begin, end, 0, 0, out);
    return out;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the range, the depth, then the halvings
std::uint32_t TemporalBuilder::node(std::vector<MovingReference> references, float begin, float end, int depth, int halvings,
    MovingBinary & out) const {
    const std::size_t count = references.size();
    const LinearBox box = boundsOf(references);
    if (count <= 1 || depth >= kDeepest) {
        return leaf(references, box, begin, end, out);
    }

    Split best;
    best.cost = std::numeric_limits<float>::infinity();
    objectSplit(references, best);

    // costs are in tests of one primitive, a step costing as much as one test
    const float area = box.area();
    const float leafCost = static_cast<float>(count);

    // a split by time keeps every reference on both sides, each over half the range. Motion that
    // turns back holds the same bounds at both ends, so only bounding each half again tells. It
    // doubles the references, so it is taken only where it costs less than a leaf, and only a
    // few times on one path
    const float middle = (begin + end) * 0.5F;
    std::vector<MovingReference> early;
    std::vector<MovingReference> late;
    if (options_.temporal && halvings < kHalvingsMost && middle > begin && middle < end && area > 0.0F) {
        early = within(references, begin, middle);
        late = within(references, middle, end);
        const float cost = 0.5F * static_cast<float>(count) * (boundsOf(early).area() + boundsOf(late).area());
        if (cost < best.cost && 1.0F + (cost / area) < leafCost) {
            best = Split { .temporal = true, .cost = cost };
        }
    }

    const float splitCost = area > 0.0F ? 1.0F + (best.cost / area) : std::numeric_limits<float>::infinity();
    const bool split = std::isfinite(best.cost);
    if (count <= kLeafMost && (!split || leafCost <= splitCost)) {
        return leaf(references, box, begin, end, out);
    }

    // a split by centroids keeps the range on both sides, and one by time halves it
    std::array<std::vector<MovingReference>, 2> halves;
    std::array<std::pair<float, float>, 2> ranges { std::pair { begin, end }, std::pair { begin, end } };
    const int below = split && best.temporal ? halvings + 1 : halvings;
    if (split && best.temporal) {
        halves = { std::move(early), std::move(late) };
        ranges = { std::pair { begin, middle }, std::pair { middle, end } };
    } else {
        if (split) {
            partition(references, best, halves[0], halves[1]);
        }
        if (halves[0].empty() || halves[1].empty()) {
            // no split separates them, as when every centroid is at one place: halve them in order
            halves[0].assign(references.begin(), references.begin() + static_cast<std::ptrdiff_t>(count / 2));
            halves[1].assign(references.begin() + static_cast<std::ptrdiff_t>(count / 2), references.end());
        }
    }
    references.clear();
    references.shrink_to_fit();

    const auto at = static_cast<std::uint32_t>(out.nodes.size());
    out.nodes.push_back(MovingBinaryNode { .box = box, .begin = begin, .end = end });
    std::array<std::uint32_t, 2> children {};
    if (count >= kParallel) {
        std::array<MovingBinary, 2> parts;
        device_.each(2, [&](std::size_t half) {
            node(std::move(halves[half]), ranges[half].first, ranges[half].second, depth + 1, below, parts[half]);
        });
        children[0] = join(parts[0], out);
        children[1] = join(parts[1], out);
    } else {
        for (std::size_t half = 0; half < 2; ++half) {
            children[half] = node(std::move(halves[half]), ranges[half].first, ranges[half].second, depth + 1, below, out);
        }
    }
    out.nodes[at].left = children[0];
    out.nodes[at].right = children[1];
    return at;
}

bool TemporalBuilder::objectSplit(const std::vector<MovingReference> & references, Split & best) {
    Box centroids;
    for (const MovingReference & reference : references) {
        centroids.grow(reference.box.at(0.5F).centre());
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
        for (const MovingReference & reference : references) {
            const float centroid = reference.box.at(0.5F).centre()[axis];
            const auto bin = std::min(static_cast<std::uint32_t>((centroid - lower) * scale), kBins - 1);  // checked: in the centroids' box, so 0 to kBins
            bins.boxes[bin].grow(reference.box);
            ++bins.counts[bin];
        }
        // the area and count right of each bin's far edge
        std::array<float, kBins> rightArea {};
        std::array<std::size_t, kBins> rightCount {};
        LinearBox right;
        std::size_t rightTotal = 0;
        for (std::uint32_t bin = kBins - 1; bin > 0; --bin) {
            right.grow(bins.boxes[bin]);
            rightTotal += bins.counts[bin];
            rightArea[bin - 1] = right.area();
            rightCount[bin - 1] = rightTotal;
        }
        LinearBox left;
        std::size_t leftTotal = 0;
        for (std::uint32_t bin = 0; bin + 1 < kBins; ++bin) {
            left.grow(bins.boxes[bin]);
            leftTotal += bins.counts[bin];
            if (leftTotal == 0 || rightCount[bin] == 0) {
                continue;
            }
            const float cost = (left.area() * static_cast<float>(leftTotal)) + (rightArea[bin] * static_cast<float>(rightCount[bin]));
            if (cost < best.cost) {
                best = Split { .temporal = false, .axis = axis, .bin = bin, .cost = cost, .lower = lower, .scale = scale };
                found = true;
            }
        }
    }
    return found;
}

std::vector<MovingReference> TemporalBuilder::within(const std::vector<MovingReference> & references, float begin, float end) const {
    std::vector<MovingReference> out;
    out.reserve(references.size());
    for (const MovingReference & reference : references) {
        out.push_back(MovingReference { .box = sweep_(reference.primitive, begin, end), .primitive = reference.primitive });
    }
    return out;
}

void TemporalBuilder::partition(const std::vector<MovingReference> & references, const Split & split, std::vector<MovingReference> & left,
    std::vector<MovingReference> & right) {
    for (const MovingReference & reference : references) {
        const float centroid = reference.box.at(0.5F).centre()[split.axis];
        const auto bin = std::min(static_cast<std::uint32_t>((centroid - split.lower) * split.scale), kBins - 1);  // checked: in the centroids' box, so 0 to kBins
        (bin <= split.bin ? left : right).push_back(reference);
    }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the range's begin, then its end
std::uint32_t TemporalBuilder::leaf(const std::vector<MovingReference> & references, const LinearBox & box, float begin, float end, MovingBinary & out) {
    const auto at = static_cast<std::uint32_t>(out.nodes.size());
    MovingBinaryNode node { .box = box, .begin = begin, .end = end, .leaf = true };
    node.first = static_cast<std::uint32_t>(out.references.size());
    node.count = static_cast<std::uint32_t>(references.size());
    for (const MovingReference & reference : references) {
        out.references.push_back(reference.primitive);
    }
    out.nodes.push_back(node);
    return at;
}

std::uint32_t TemporalBuilder::join(const MovingBinary & part, MovingBinary & out) {
    const auto nodeOffset = static_cast<std::uint32_t>(out.nodes.size());
    const auto referenceOffset = static_cast<std::uint32_t>(out.references.size());
    for (MovingBinaryNode node : part.nodes) {
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

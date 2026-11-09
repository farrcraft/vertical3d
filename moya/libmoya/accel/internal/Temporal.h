/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Builder.h>
#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/device/CpuDevice.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace v3d::moya::accel::internal {

/** A primitive as the builder sorts it, with its bounds over the time range of its node. **/
struct MovingReference {
    LinearBox box;
    std::uint32_t primitive { 0 };
};

/** A node of the binary hierarchy before it is collapsed, over a time range. **/
struct MovingBinaryNode {
    LinearBox box;
    float begin { 0.0F };
    float end { 0.0F };
    bool leaf { false };
    std::uint32_t left { 0 };
    std::uint32_t right { 0 };
    std::uint32_t first { 0 };
    std::uint32_t count { 0 };
};

/** A binary hierarchy over moving primitives, or part of one: its root is its first node. **/
struct MovingBinary {
    std::vector<MovingBinaryNode> nodes;
    std::vector<std::uint32_t> references;
};

/**
 * Builds a binary hierarchy over moving primitives by the surface area heuristic, splitting each
 * node by its primitives or by time. buildMoving() describes the rules. A node of many references
 * builds its two halves on the device's threads, each into a part of its own, and joins them in
 * order, so the result does not depend on the threads.
 **/
class TemporalBuilder final {
 public:
    TemporalBuilder(device::CpuDevice & device, const Sweep & sweep, const Options & options);

    /** The hierarchy over the references, each bounded over the time range. **/
    MovingBinary build(std::vector<MovingReference> references, float begin, float end) const;

 private:
    /** How a node is split: by time at its middle, or by centroids at a bin's far edge. **/
    struct Split {
        bool temporal { false };
        int axis { 0 };
        std::uint32_t bin { 0 };
        float cost { 0.0F };

        /** Where a split by centroids starts its bins along the axis, and bins per unit. **/
        float lower { 0.0F };
        float scale { 0.0F };
    };

    /** The bins of a split by centroids. **/
    static constexpr std::uint32_t kBins = 32;

    /** The references along an axis, by the bin of their centroid at the middle of the range. **/
    struct Bins {
        std::array<LinearBox, kBins> boxes;
        std::array<std::size_t, kBins> counts {};
    };

    /** The most references a leaf holds, unless every split fails. **/
    static constexpr std::size_t kLeafMost = 8;

    /** Nodes of at least this many references build their halves on separate threads. **/
    static constexpr std::size_t kParallel = 4096;

    /** The deepest a hierarchy grows. A node there is a leaf, however many references it holds. **/
    static constexpr int kDeepest = 64;

    /** The most times one path from the root halves its time range, splitting it by time. **/
    static constexpr int kHalvingsMost = 8;

    /**
     * Adds the hierarchy over the references to out, and returns the index of its root. Halvings
     * counts the splits by time above the node.
     **/
    std::uint32_t node(std::vector<MovingReference> references, float begin, float end, int depth, int halvings, MovingBinary & out) const;

    /** The best split by centroids, or none when every centroid is at one place. **/
    static bool objectSplit(const std::vector<MovingReference> & references, Split & best);

    /** The references bounded again over a part of the range. **/
    std::vector<MovingReference> within(const std::vector<MovingReference> & references, float begin, float end) const;

    /** The references on each side of a split by centroids. **/
    static void partition(const std::vector<MovingReference> & references, const Split & split, std::vector<MovingReference> & left,
        std::vector<MovingReference> & right);

    /** Adds a leaf of the references to out, and returns its index. **/
    static std::uint32_t leaf(const std::vector<MovingReference> & references, const LinearBox & box, float begin, float end, MovingBinary & out);

    /** Adds a part built apart to out, its indices moved past what out holds. Gives its root. **/
    static std::uint32_t join(const MovingBinary & part, MovingBinary & out);

    device::CpuDevice & device_;
    Sweep sweep_;
    Options options_;
};

};  // namespace v3d::moya::accel::internal

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Builder.h>
#include <moya/libmoya/accel/Options.h>
#include <moya/libmoya/accel/internal/Binary.h>
#include <moya/libmoya/accel/internal/Reference.h>
#include <moya/libmoya/device/CpuDevice.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace v3d::moya::accel::internal {

/**
 * Builds a binary hierarchy by the surface area heuristic, with spatial splits. Build() describes
 * the rules. A node of many references builds its two halves on the device's threads, each into a
 * part of its own, and joins them in order, so the result does not depend on the threads.
 **/
class BinaryBuilder final {
 public:
    BinaryBuilder(device::CpuDevice & device, const Clip & clip, const Options & options, float rootArea);

    /**
     * The hierarchy over the references, which lie in the box, with a budget of references spatial
     * splits may add.
     **/
    Binary build(std::vector<Reference> references, const Box & box, std::size_t budget) const;

 private:
    /** How a node is split: by centroids or in space, along an axis, at a bin's far edge. **/
    struct Split {
        bool spatial { false };
        int axis { 0 };
        std::uint32_t bin { 0 };
        float cost { 0.0F };
        Box left;
        Box right;
        std::size_t added { 0 };

        /** Where a split by centroids starts its bins along the axis, and bins per unit. **/
        float lower { 0.0F };
        float scale { 0.0F };
    };

    /** The bins of each split. **/
    static constexpr std::uint32_t kBins = 32;

    /**
     * The references along an axis, by bin: each bin's box, and how many references start and end
     * in it. A split by centroids counts each reference as starting and ending in its centroid's
     * bin.
     **/
    struct Bins {
        std::array<Box, kBins> boxes;
        std::array<std::size_t, kBins> entering {};
        std::array<std::size_t, kBins> leaving {};
    };

    /** The most references a leaf holds, unless every split fails. **/
    static constexpr std::size_t kLeafMost = 8;

    /** Nodes of at least this many references build their halves on separate threads. **/
    static constexpr std::size_t kParallel = 4096;

    /** The deepest a hierarchy grows. A node there is a leaf, however many references it holds. **/
    static constexpr int kDeepest = 64;

    /** Spatial splits are tried past this overlap of a split's halves, over the root's area. **/
    static constexpr float kOverlap = 1.0e-5F;

    /** Adds the hierarchy over the references to out, and returns the index of its root. **/
    std::uint32_t node(std::vector<Reference> references, const Box & box, std::size_t budget, int depth, Binary & out) const;

    /** The best split by centroids, or none when every centroid is at one place. **/
    static bool objectSplit(const std::vector<Reference> & references, Split & best);

    /** The best split in space within a budget of added references, or none. **/
    bool spatialSplit(const std::vector<Reference> & references, const Box & box, std::size_t budget, Split & best) const;

    /** A spatial split's bins along an axis, each reference clipped to every bin it crosses. **/
    Bins spatialBins(const std::vector<Reference> & references, const Box & box, int axis) const;

    /**
     * Makes best the cheapest split at a bin's far edge, when it costs less than best does and adds
     * no more references than the budget. The candidate gives the rest of the split.
     **/
    static bool sweep(const Bins & bins, std::size_t total, std::size_t budget, const Split & candidate, Split & best);

    /** The references on each side of a split. **/
    void partition(std::vector<Reference> & references, const Box & box, const Split & split, std::vector<Reference> & left,
        std::vector<Reference> & right) const;

    /** Adds a leaf of the references to out, and returns its index. **/
    static std::uint32_t leaf(const std::vector<Reference> & references, const Box & box, Binary & out);

    /** Adds a part built apart to out, its indices moved past what out holds. Gives its root. **/
    static std::uint32_t join(const Binary & part, Binary & out);

    device::CpuDevice & device_;
    Clip clip_;
    Options options_;
    float rootArea_;
};

};  // namespace v3d::moya::accel::internal

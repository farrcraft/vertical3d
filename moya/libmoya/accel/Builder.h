/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Node.h>
#include <moya/libmoya/device/CpuDevice.h>

#include <cstdint>
#include <functional>
#include <vector>

namespace v3d::moya::accel {

/**
 * A built hierarchy, in host memory. The root is the first node, and a scene of no primitives has
 * one node whose children are all empty. A leaf's references are primitive indices, and a
 * primitive split in space is referenced from each leaf that holds part of it.
 **/
struct Hierarchy {
    std::vector<Node> nodes;
    std::vector<std::uint32_t> references;

    /** The box of everything the hierarchy holds. **/
    Box bounds;
};

/** The bounds of the part of a primitive inside a box, which a spatial split needs. **/
using Clip = std::function<Box(std::uint32_t primitive, const Box & within)>;

/** How a hierarchy is built. **/
struct Options {
    /** Whether a primitive may be split in space, so that it is referenced from two leaves. **/
    bool spatial { true };

    /**
     * How many references spatial splits may add, as a fraction of the primitives. Each node
     * shares its own part among its children by their counts, so the budget does not depend on
     * which thread builds which node.
     **/
    float budget { 1.0F };
};

/**
 * Builds a hierarchy over primitives by their bounds.
 *
 * Each node is split by the surface area heuristic, over 32 bins of the centroids along each
 * axis. Where the two halves of the best such split overlap, the node also tries 32 bins in
 * space along each axis. These clip each primitive to the bins it crosses, as Stich, Friedrich
 * and Dietrich's split hierarchy does. A node of eight primitives or fewer becomes a leaf when that
 * costs less than splitting it.
 *
 * The binary hierarchy is then collapsed to nodes of eight children, each node opening its child
 * of the largest area until it holds eight. The result depends only on the primitives, not on
 * the device's threads or the order they finish in.
 *
 * @param bounds each primitive's box
 * @param clip the bounds of part of a primitive, for spatial splits
 * @throws std::invalid_argument when the budget is not finite or is negative, or there are 2^31
 *         primitives or more, which a leaf cannot name
 **/
Hierarchy build(device::CpuDevice & device, const std::vector<Box> & bounds, const Clip & clip, const Options & options = {});

/**
 * A hierarchy's cost by the surface area heuristic: each node's area times the cost of a step, and
 * each leaf's area times its references, each over the root's area. A step costs as much as a
 * test of one primitive.
 **/
double cost(const Hierarchy & hierarchy);

};  // namespace v3d::moya::accel

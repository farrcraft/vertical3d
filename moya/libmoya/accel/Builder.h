/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Hierarchy.h>
#include <moya/libmoya/accel/MovingHierarchy.h>
#include <moya/libmoya/accel/Options.h>
#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/device/CpuDevice.h>

#include <cstdint>
#include <functional>
#include <vector>

namespace v3d::moya::accel {

/** The bounds of the part of a primitive inside a box, which a spatial split needs. **/
using Clip = std::function<Box(std::uint32_t primitive, const Box & within)>;

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

/**
 * Builds a hierarchy over moving primitives by their bounds over time.
 *
 * Each node covers a time range, and each of its primitives is bounded over that range by a box
 * at each end. A node is split by the surface area heuristic, over 32 bins of the centroids at
 * the middle of its range, or by time at the middle of its range. A split by time keeps every
 * primitive on both sides, each bounded again over its half. It is taken where it costs less,
 * as where motion turns back and its bounds over the whole range are loose, and where it also
 * costs less than a leaf. No path from the root splits by time more than eight times. A node of
 * more than eight primitives that no split helps is halved in order. There are no spatial
 * splits. A box's area is its area averaged over its range.
 *
 * The binary hierarchy is then collapsed to nodes of eight children as build() collapses one,
 * opening the child of the largest area times its share of the range. The result depends only on
 * the primitives.
 *
 * @param count how many primitives there are
 * @param sweep each primitive's bounds over a time range
 * @throws std::invalid_argument when the range is not finite or ends before it begins, or there are
 *         2^31 primitives or more
 **/
MovingHierarchy buildMoving(device::CpuDevice & device, std::uint32_t count, const Sweep & sweep, float begin, float end,
    const Options & options = {});

/**
 * A moving hierarchy's cost by the surface area heuristic, as cost() gives a still one's, with each
 * child's area also weighted by its share of the hierarchy's time range.
 **/
double cost(const MovingHierarchy & hierarchy);

};  // namespace v3d::moya::accel

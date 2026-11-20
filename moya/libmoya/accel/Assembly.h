/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Root.h>
#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/device/cpu/Instance.h>
#include <moya/libmoya/device/cpu/MotionNode.h>
#include <moya/libmoya/device/cpu/Node.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/geometry/Key.h>
#include <moya/libmoya/kernel/geometry/Motion.h>

#include <cstdint>
#include <vector>

namespace v3d::moya::accel {

/**
 * A hierarchy for each geometry and each group of a scene's prototypes, and the instances that
 * place them, as the CPU traverses them. Still hierarchies are held in one array of nodes, and
 * moving ones in one of moving nodes. The instances are the prototypes', in their order.
 *
 * A geometry's hierarchy is built once, however many instances place it, and its leaves hold the
 * indices of its primitives within it. A group's leaves hold its instances. The world's hierarchy
 * is a group's.
 **/
struct Assembly {
    std::vector<device::cpu::Node> nodes;
    std::vector<device::cpu::MotionNode> motionNodes;
    std::vector<std::uint32_t> references;
    std::vector<device::cpu::Instance> instances;

    /** The keys of every instance that moves. **/
    std::vector<kernel::geometry::Key> keys;

    /** The root of each geometry's hierarchy, and of each group's, in the prototypes' order. **/
    std::vector<Root> geometries;
    std::vector<Root> groups;
    Root world;

    /** The bounds of everything the world places over the shutter. **/
    LinearBox bounds;
};

/**
 * Builds the hierarchies of a scene's prototypes.
 *
 * A geometry that does not move is built as build() builds one: a mesh over its triangles, with
 * spatial splits, and points over their boxes, without. One that moves is built as buildMoving()
 * builds one, over the shutter. A quadric is one primitive, bounded as quadricBounds() bounds it.
 * A group moves when an instance in it moves, or places something that moves. A group that does
 * not is built over its instances' boxes, each its prototype's box moved by its transformation.
 * One that does is built as buildMoving() builds one. Neither has spatial splits.
 *
 * An instance in a moving group is bounded over each time range the builder asks for, by its
 * prototype's bounds over that range:
 * - an instance that holds still moves its prototype's box at each end of the range.
 * - one that moves sweeps its prototype's box over the whole range along its transformation, as
 *   sweepBox() does.
 *
 * @throws std::length_error when the hierarchies hold more nodes or references than a child can
 *         name
 **/
Assembly assemble(device::CpuDevice & device, const geometry::Prototypes & prototypes);

};  // namespace v3d::moya::accel

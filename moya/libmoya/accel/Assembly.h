/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/device/cpu/Instance.h>
#include <moya/libmoya/device/cpu/Node.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/geometry/Prototypes.h>

#include <cstdint>
#include <vector>

namespace v3d::moya::accel {

/**
 * A hierarchy for each mesh and each group of a scene's prototypes, held in one array of nodes,
 * and the instances that place them, as the CPU traverses them. The instances are the
 * prototypes', in their order.
 *
 * A mesh's hierarchy is built once, however many instances place it, and its leaves hold the
 * indices of its triangles within it. A group's leaves hold its instances, each bounded by its
 * prototype's box moved by its transformation. The world's hierarchy is a group's.
 **/
struct Assembly {
    std::vector<device::cpu::Node> nodes;
    std::vector<std::uint32_t> references;
    std::vector<device::cpu::Instance> instances;

    /** The root of each mesh's hierarchy, and of each group's, in the prototypes' order. **/
    std::vector<std::uint32_t> meshes;
    std::vector<std::uint32_t> groups;

    /** The root of the world's hierarchy. **/
    std::uint32_t world { 0 };

    /** The box of everything the world places. **/
    Box bounds;
};

/**
 * Builds the hierarchies of a scene's prototypes. Each mesh's is built as build() builds one over
 * triangles, and each group's over its instances' boxes, with no spatial splits.
 *
 * @throws std::length_error when the hierarchies hold more nodes or references than a child can
 *         name
 **/
Assembly assemble(device::CpuDevice & device, const geometry::Prototypes & prototypes);

};  // namespace v3d::moya::accel

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/device/cpu/Node.h>

#include <cstdint>
#include <vector>

namespace v3d::moya::accel {

/**
 * A built hierarchy, in host memory. The root is the first node, and a scene of no primitives has
 * one node whose children are all empty. A leaf's references are primitive indices, and a
 * primitive split in space is referenced from each leaf that holds part of it.
 **/
struct Hierarchy {
    std::vector<device::cpu::Node> nodes;
    std::vector<std::uint32_t> references;

    /** The box of everything the hierarchy holds. **/
    Box bounds;
};

};  // namespace v3d::moya::accel

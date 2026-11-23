/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/device/cpu/MotionNode.h>
#include <moya/libmoya/device/cpu/Node.h>

#include <cstdint>
#include <vector>

namespace v3d::moya::accel::internal {

/**
 * The hierarchy of one prototype, apart from every other's, so that it can be placed in an
 * assembly wherever its nodes land. Its children count from its own first node and reference, and
 * its references from its own primitives: a geometry's primitives, or a group's members by their
 * place in the group.
 **/
struct Part {
    bool moving { false };

    /** Whether it was built as a geometry that is cut out. **/
    bool cutOut { false };

    /** A still hierarchy's nodes, or a moving one's. **/
    std::vector<device::cpu::Node> nodes;
    std::vector<device::cpu::MotionNode> motionNodes;
    std::vector<std::uint32_t> references;

    /** Its bounds over the shutter. **/
    LinearBox bounds;
};

};  // namespace v3d::moya::accel::internal

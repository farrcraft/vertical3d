/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <limits>

namespace v3d::moya::device::cpu {

/** The most children a node holds. **/
inline constexpr std::uint32_t kWidth = 8;

/** A child slot that holds nothing. Its box is empty, so no ray reaches it. **/
inline constexpr std::uint32_t kEmpty = std::numeric_limits<std::uint32_t>::max();

/** The bit of a child that marks a leaf. The rest is the leaf's first reference. **/
inline constexpr std::uint32_t kLeaf = 0x80000000U;

/**
 * A node of the hierarchy the CPU traverses: up to eight children, each a box at full precision.
 * The boxes are stored axis by axis, each array holding one plane of every child, so a SIMD test
 * reads one plane of all eight at once.
 *
 * A child is the index of another node, or kLeaf with the index of the leaf's first reference
 * and count[] references, or kEmpty with an empty box. A node's children are packed from the
 * first slot.
 **/
struct alignas(32) Node {
    float lowerX[kWidth];
    float upperX[kWidth];
    float lowerY[kWidth];
    float upperY[kWidth];
    float lowerZ[kWidth];
    float upperZ[kWidth];
    std::uint32_t child[kWidth];
    std::uint32_t count[kWidth];
};

/**
 * A node of a hierarchy over geometry that moves. Each child holds a box at the start and at the
 * end of the time range it covers, and the box at a time between is interpolated linearly. A ray
 * outside a child's range does not reach it.
 *
 * The planes at the start are stored as a Node stores its planes, and those at the end after
 * them. A child is linked as in a Node.
 **/
struct alignas(32) MotionNode {
    float lowerX[kWidth];
    float upperX[kWidth];
    float lowerY[kWidth];
    float upperY[kWidth];
    float lowerZ[kWidth];
    float upperZ[kWidth];
    float endLowerX[kWidth];
    float endUpperX[kWidth];
    float endLowerY[kWidth];
    float endUpperY[kWidth];
    float endLowerZ[kWidth];
    float endUpperZ[kWidth];
    float begin[kWidth];
    float end[kWidth];
    std::uint32_t child[kWidth];
    std::uint32_t count[kWidth];
};

};  // namespace v3d::moya::device::cpu

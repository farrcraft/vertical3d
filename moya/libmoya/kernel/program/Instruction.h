/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/program/Op.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

namespace v3d::moya::kernel::program {

/** The most operands an instruction reads. **/
inline constexpr std::uint32_t kMostOperands = 5;

/**
 * One node of a material's program: its operation, the registers of its output, and the
 * registers of each operand it reads. A value of n components lies in n registers from its
 * first. An operation takes as many operands as its node has inputs, in the order Op gives them.
 **/
struct Instruction {
    Op op { Op::Copy };

    /** The output's first register and its components, one to four. **/
    std::uint32_t out { 0 };
    std::uint32_t width { 1 };

    /** Each operand's first register and its components. **/
    std::uint32_t in[kMostOperands] {};
    std::uint32_t widths[kMostOperands] {};
};

/** An operand's component, or its last component where it has fewer, so one component spreads. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the operand, then the component
MOYA_KERNEL inline float operand(const float * registers, const Instruction & instruction, std::uint32_t index, std::uint32_t component) {
    const std::uint32_t width = instruction.widths[index];
    std::uint32_t at = component;
    if (width <= component) {
        at = width > 0U ? width - 1U : 0U;
    }
    return registers[instruction.in[index] + at];
}

};  // namespace v3d::moya::kernel::program

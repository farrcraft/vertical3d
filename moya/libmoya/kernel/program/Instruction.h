/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/program/Op.h>

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

};  // namespace v3d::moya::kernel::program

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::kernel::program {

/** The registers a program may use, which a shading point holds while it runs. **/
inline constexpr std::uint32_t kRegisters = 256;

/**
 * A material's program: its instructions, which run in order, and the values its registers hold
 * before the first runs. The values begin with the surface's parameters, laid out as
 * Parameters.h says, and go on to the program's constants and the outputs of its nodes. A program
 * of no instructions gives the surface's parameters as its values hold them.
 **/
struct Program {
    std::uint32_t instructions { 0 };
    std::uint32_t count { 0 };
    std::uint32_t values { 0 };
    std::uint32_t size { 0 };
};

};  // namespace v3d::moya::kernel::program

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

/**
 * What the CPU's libraries share with libmoya: the node layout the CPU traverses, the traversal,
 * and each library's table of entry points. Each library builds its own instruction set's code
 * from these headers, which hold no instruction-specific type.
 **/
namespace v3d::moya::device::cpu {

/**
 * An instruction set the CPU's fused path is built for, oldest first. Each is the x86-64 level
 * of the same name's newest extension: SSE4.2 is x86-64-v2, AVX2 is x86-64-v3, and AVX-512 is
 * x86-64-v4.
 **/
enum class Isa : std::uint8_t {
    Sse42,
    Avx2,
    Avx512
};

};  // namespace v3d::moya::device::cpu

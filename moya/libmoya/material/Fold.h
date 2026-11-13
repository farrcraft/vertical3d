/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/program/Instruction.h>

#include <cstdint>
#include <span>
#include <vector>

namespace v3d::moya::material {

/**
 * A program's registers as they stand after it runs, where they are the same at every shading
 * point. A register is constant when it starts from a value and every instruction that writes it
 * reads only constant registers. An instruction that reads the shading point or an image varies.
 * Each constant register holds its value, and every other register the value it starts from.
 **/
struct Folded {
    std::vector<float> values;
    std::vector<bool> constant;

    /** Whether the registers from a first, of a width, are each constant. **/
    bool constantFrom(std::uint32_t first, std::uint32_t width) const;
};

/** A program of instructions, from the values its registers start from, folded on the host. **/
Folded fold(std::span<const kernel::program::Instruction> instructions, std::span<const float> values);

};  // namespace v3d::moya::material

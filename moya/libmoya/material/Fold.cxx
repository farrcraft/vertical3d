/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/material/Fold.h>

#include <moya/libmoya/kernel/program/Instruction.h>
#include <moya/libmoya/kernel/program/Op.h>
#include <moya/libmoya/kernel/program/Program.h>
#include <moya/libmoya/kernel/program/Run.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/kernel/texture/Untextured.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace v3d::moya::material {

namespace {

using kernel::program::Instruction;
using kernel::program::Op;

/** Whether an operation reads the shading point or an image, so it varies over a surface. **/
bool readsThePoint(Op op) {
    return op == Op::Image || op == Op::Shift || static_cast<std::uint32_t>(op) >= static_cast<std::uint32_t>(Op::Primvar);
}

};  // namespace

bool Folded::constantFrom(std::uint32_t first, std::uint32_t width) const {
    for (std::uint32_t at = first; at < first + width; ++at) {
        if (at >= constant.size() || !constant[at]) {
            return false;
        }
    }
    return true;
}

Folded fold(std::span<const Instruction> instructions, std::span<const float> values) {
    Folded out;
    out.values.assign(kernel::program::kRegisters, 0.0F);
    out.constant.assign(kernel::program::kRegisters, false);
    for (std::size_t at = 0; at < values.size() && at < out.values.size(); ++at) {
        out.values[at] = values[at];
        out.constant[at] = true;
    }
    // an operation that reads neither the point nor an image reads only its operands, so it runs
    // on the host over no surfaces
    const kernel::surface::Surfaces none;
    const kernel::surface::ShadingPoint nowhere;
    for (const Instruction & instruction : instructions) {
        // an instruction that writes outside the registers, as a program too large has, is not run
        bool constant = !readsThePoint(instruction.op) && instruction.out + instruction.width <= kernel::program::kRegisters;
        for (std::uint32_t operand = 0; operand < kernel::program::kMostOperands && constant; ++operand) {
            constant = instruction.widths[operand] == 0U || out.constantFrom(instruction.in[operand], instruction.widths[operand]);
        }
        if (constant) {
            kernel::program::execute(instruction, out.values.data(), none, kernel::texture::Untextured {}, nowhere);
        }
        for (std::uint32_t component = 0; component < instruction.width; ++component) {
            if (instruction.out + component < out.constant.size()) {
                out.constant[instruction.out + component] = constant;
            }
        }
    }
    return out;
}

};  // namespace v3d::moya::material

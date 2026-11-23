/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/program/Instruction.h>
#include <moya/libmoya/kernel/program/Medium.h>
#include <moya/libmoya/kernel/program/Op.h>
#include <moya/libmoya/kernel/program/Program.h>
#include <moya/libmoya/kernel/program/Run.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/kernel/texture/Texels.h>
#include <moya/libmoya/kernel/volume/Medium.h>

#include <cstdint>

namespace v3d::moya::kernel::program {

/** The value of a field at a point, and the number of the primvar a program reads it as. **/
struct FieldValue {
    std::uint32_t name { 0 };
    float value { 0.0F };
};

/**
 * Runs a volume's program at a point in a medium, with the values of the fields the volume reads.
 * A Primvar instruction reads the field it numbers, the same value in each of its components, and
 * gives its default where the volume reads no such field. A point in a medium has no footprint,
 * so a Shift moves nothing. Every other instruction runs as run() runs it.
 **/
template <texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the fields, then how many
MOYA_KERNEL void runWithFields(const surface::Surfaces & surfaces, const X & textures, const surface::ShadingPoint & point, const Program & program,
    const FieldValue * fields, std::uint32_t count, float * registers) {
    for (std::uint32_t i = 0; i < program.size && i < kRegisters; ++i) {
        registers[i] = surfaces.programValues[program.values + i];
    }
    for (std::uint32_t i = 0; i < program.count; ++i) {
        const Instruction & instruction = surfaces.instructions[program.instructions + i];
        if (instruction.op == Op::Shift) {
            continue;
        }
        if (instruction.op != Op::Primvar) {
            execute(instruction, registers, surfaces, textures, point);
            continue;
        }
        const float * given = registers + instruction.in[1];
        float read[kMostComponents] {};
        bool found = false;
        for (std::uint32_t field = 0; field < count; ++field) {
            if (fields[field].name == instruction.in[0]) {
                found = true;
                for (std::uint32_t component = 0; component < instruction.width && component < kMostComponents; ++component) {
                    read[component] = fields[field].value;
                }
            }
        }
        for (std::uint32_t component = 0; component < instruction.width && component < kMostComponents; ++component) {
            registers[instruction.out + component] = found ? read[component] : given[component];
        }
    }
}

/** The medium a volume's program gives at a point in it, from the values of its fields. **/
template <texture::Texels X>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the fields, then how many
MOYA_KERNEL volume::Medium mediumWithFields(const surface::Surfaces & surfaces, const X & textures, const surface::ShadingPoint & point, const Program & program,
    const FieldValue * fields, std::uint32_t count) {
    float registers[kRegisters] {};
    const std::uint64_t mark = texture::started(textures);
    runWithFields(surfaces, textures, point, program, fields, count, registers);
    texture::finished(textures, texture::Work::Program, mark);
    return mediumOf(registers);
}

};  // namespace v3d::moya::kernel::program

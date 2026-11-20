/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/program/Instruction.h>
#include <moya/libmoya/kernel/program/Noise.h>
#include <moya/libmoya/kernel/program/Op.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>
#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::kernel::program {

/** Whether an operation is a pattern, which Pattern.h computes. **/
MOYA_KERNEL inline bool patterned(Op op) {
    return static_cast<std::uint32_t>(op) >= static_cast<std::uint32_t>(Op::Noise) && static_cast<std::uint32_t>(op) <= static_cast<std::uint32_t>(Op::Rotate3d);
}

/**
 * A noise or a fractal of a point, of one to four channels, before its amplitude. One channel is
 * the noise of one, and two or three are the first channels of the noise of three. A fractal's two
 * are instead two fractals of one, the second at an offset point. Four are three channels and the
 * noise of one at an offset point.
 **/
template <glm::length_t N>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point, then the two offsets
MOYA_KERNEL inline glm::vec4 noiseAt(const Instruction & instruction, const float * registers, const NoisePoint<N> & point, const NoisePoint<N> & fourth,
    const NoisePoint<N> & later) {
    const std::uint32_t width = instruction.width;
    const std::uint32_t channels = width < 4U ? width : 3U;
    if (instruction.op == Op::Noise) {
        return { perlin<N>(point, channels), width == 4U ? perlin<N>(point + fourth, 1U).x : 0.0F };
    }
    const std::int32_t octaves = wholeOf(std::fmin(std::fmax(registers[instruction.in[1]], 0.0F), static_cast<float>(kMostOctaves)));
    const float lacunarity = registers[instruction.in[2]];
    const float diminish = registers[instruction.in[3]];
    if (width == 2U) {
        return { fractal<N>(point, 1U, octaves, lacunarity, diminish).x, fractal<N>(point + later, 1U, octaves, lacunarity, diminish).x, 0.0F, 0.0F };
    }
    return { fractal<N>(point, channels, octaves, lacunarity, diminish), width == 4U ? fractal<N>(point + later, 1U, octaves, lacunarity, diminish).x : 0.0F };
}

/** A noise or a fractal of a position of two components or of three, before its amplitude. **/
MOYA_KERNEL inline glm::vec4 noiseOf(const Instruction & instruction, const float * registers) {
    const std::uint32_t position = instruction.op == Op::Noise ? 2U : 4U;
    const float * at = registers + instruction.in[position];
    if (instruction.widths[position] == 2U) {
        return noiseAt<2>(instruction, registers, glm::vec2 { at[0], at[1] }, kFourthNoise2, kLaterFractal2);
    }
    return noiseAt<3>(instruction, registers, glm::vec3 { at[0], at[1], at[2] }, kFourthNoise3, kLaterFractal3);
}

/** A cellular pattern: cell noise, or Worley noise. **/
MOYA_KERNEL inline glm::vec3 cellsOf(const Instruction & instruction, const float * registers) {
    const float * at = registers + instruction.in[0];
    const bool flat = instruction.widths[0] == 2U;
    const glm::vec2 p2 { at[0], at[1] };
    const glm::vec3 p3 { at[0], at[1], flat ? 0.0F : at[2] };
    if (instruction.op == Op::CellNoise) {
        return glm::vec3(flat ? cell<2>(p2) : cell<3>(p3));
    }
    const float jitter = registers[instruction.in[1]];
    const bool solid = registers[instruction.in[2]] == 1.0F;
    return flat ? worley<2>(p2, jitter, solid, instruction.width) : worley<3>(p3, jitter, solid, instruction.width);
}

/** A vector turned by rotate2d or rotate3d. **/
MOYA_KERNEL inline glm::vec3 rotatedOf(const Instruction & instruction, const float * registers) {
    const float * given = registers + instruction.in[0];
    const float angle = glm::radians(registers[instruction.in[1]]);
    const float sine = std::sin(angle);
    const float cosine = std::cos(angle);
    if (instruction.op == Op::Rotate2d) {
        return { (cosine * given[0]) + (sine * given[1]), (cosine * given[1]) - (sine * given[0]), 0.0F };
    }
    const glm::vec3 in { given[0], given[1], given[2] };
    const float * held = registers + instruction.in[2];
    const glm::vec3 axis { held[0], held[1], held[2] };
    if (glm::dot(axis, axis) <= 0.0F) {
        return in;
    }
    const glm::vec3 unit = glm::normalize(axis);
    return (in * cosine) + (glm::cross(in, unit) * sine) + (unit * glm::dot(unit, in) * (1.0F - cosine));
}

/**
 * A ramp or a split: from a first value to a second across a texture coordinate. A ramp mixes them
 * by the coordinate held to [0, 1]. A split gives the second from its center on.
 **/
MOYA_KERNEL inline void acrossOf(const Instruction & instruction, const float * registers, float * out) {
    const bool split = instruction.op == Op::SplitLeftRight || instruction.op == Op::SplitTopBottom;
    const bool across = instruction.op == Op::RampLeftRight || instruction.op == Op::SplitLeftRight;
    const float * texcoord = registers + instruction.in[split ? 3U : 2U];
    const float coordinate = across ? texcoord[0] : texcoord[1];
    float t = std::fmin(std::fmax(coordinate, 0.0F), 1.0F);
    if (split) {
        t = coordinate < registers[instruction.in[2]] ? 0.0F : 1.0F;
    }
    // left to right mixes from the first operand, and top to bottom from the second, its bottom
    const std::uint32_t from = across ? 0U : 1U;
    const std::uint32_t to = across ? 1U : 0U;
    for (std::uint32_t component = 0; component < instruction.width && component < 4U; ++component) {
        const float low = operand(registers, instruction, from, component);
        const float high = operand(registers, instruction, to, component);
        out[component] = low + ((high - low) * t);
    }
}

/** The pattern an instruction names, into out. **/
MOYA_KERNEL inline void patternOf(const Instruction & instruction, const float * registers, float * out) {
    glm::vec4 value { 0.0F };
    switch (instruction.op) {
        case Op::Noise:
        case Op::Fractal: {
            value = noiseOf(instruction, registers);
            const float pivot = instruction.op == Op::Noise ? registers[instruction.in[1]] : 0.0F;
            for (std::uint32_t component = 0; component < instruction.width && component < 4U; ++component) {
                value[static_cast<glm::length_t>(component)] = (value[static_cast<glm::length_t>(component)] * operand(registers, instruction, 0U, component)) + pivot;
            }
            break;
        }
        case Op::CellNoise:
        case Op::Worley:
            value = glm::vec4(cellsOf(instruction, registers), 0.0F);
            break;
        case Op::Rotate2d:
        case Op::Rotate3d:
            value = glm::vec4(rotatedOf(instruction, registers), 0.0F);
            break;
        default:
            acrossOf(instruction, registers, out);
            return;
    }
    for (std::uint32_t component = 0; component < instruction.width && component < 4U; ++component) {
        out[component] = value[static_cast<glm::length_t>(component)];
    }
}

};  // namespace v3d::moya::kernel::program

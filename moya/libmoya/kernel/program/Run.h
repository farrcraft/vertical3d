/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/program/Instruction.h>
#include <moya/libmoya/kernel/program/Op.h>
#include <moya/libmoya/kernel/program/Parameters.h>
#include <moya/libmoya/kernel/program/Program.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>

#include <cmath>
#include <cstdint>

namespace v3d::moya::kernel::program {

/** The most components a value holds. **/
inline constexpr std::uint32_t kMostComponents = 4;

/** How close to zero burn's fg, and dodge's one less fg, may come before it gives zero. **/
inline constexpr float kLeastDivisor = 1.0e-8F;

/** The sixths of the circle a hue is taken in, one for each primary and secondary colour. **/
inline constexpr float kHueSectors = 6.0F;

/** Where blue's sector begins, in sixths of the circle. **/
inline constexpr float kBlueSector = 4.0F;

/** The least saturation a colour has a hue at. **/
inline constexpr float kLeastSaturation = 1.0e-4F;

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

/** A boolean as a program holds it: one for true, and zero for false. **/
MOYA_KERNEL inline float truth(bool held) {
    return held ? 1.0F : 0.0F;
}

/** An operation taken one component at a time, from that component of each of its operands. **/
// NOLINTNEXTLINE(readability-function-size) - one case for each operation, as Op lists them
MOYA_KERNEL inline float componentOf(Op op, const float * operands) {
    const float first = operands[0];
    const float second = operands[1];
    const float third = operands[2];
    const float fourth = operands[3];
    switch (op) {
        case Op::Copy:
            return first;
        case Op::Add:
            return first + second;
        case Op::Subtract:
            return first - second;
        case Op::Multiply:
            return first * second;
        case Op::Divide:
            return first / second;
        case Op::Modulo:
            return first - (second * std::floor(first / second));
        case Op::Fract:
            return first - std::floor(first);
        case Op::Abs:
            return std::fabs(first);
        case Op::Floor:
            return std::floor(first);
        case Op::Ceil:
            return std::ceil(first);
        case Op::Round:
            return std::round(first);
        case Op::Power:
            return std::pow(first, second);
        case Op::Sin:
            return std::sin(first);
        case Op::Cos:
            return std::cos(first);
        case Op::Tan:
            return std::tan(first);
        case Op::Asin:
            return std::asin(first);
        case Op::Acos:
            return std::acos(first);
        case Op::Atan2:
            return std::atan2(first, second);
        case Op::Sqrt:
            return std::sqrt(first);
        case Op::Ln:
            return std::log(first);
        case Op::Exp:
            return std::exp(first);
        case Op::Sign:
            return truth(first > 0.0F) - truth(first < 0.0F);
        case Op::Clamp:
            return std::fmin(std::fmax(first, second), third);
        case Op::Min:
            return std::fmin(first, second);
        case Op::Max:
            return std::fmax(first, second);
        case Op::Remap:
            return fourth + ((first - second) * (operands[4] - fourth) / (third - second));
        case Op::Smoothstep: {
            if (first >= third) {
                return 1.0F;
            }
            if (first <= second) {
                return 0.0F;
            }
            const float t = (first - second) / (third - second);
            return t * t * (1.0F + (2.0F * (1.0F - t)));
        }
        case Op::Mix:
            return second + ((first - second) * third);
        case Op::Plus:
            return (third * (first + second)) + ((1.0F - third) * second);
        case Op::Minus:
            return (third * (second - first)) + ((1.0F - third) * second);
        case Op::Difference:
            return (third * std::fabs(second - first)) + ((1.0F - third) * second);
        case Op::Burn:
            return std::fabs(first) < kLeastDivisor ? 0.0F : (third * (1.0F - ((1.0F - second) / first))) + ((1.0F - third) * second);
        case Op::Dodge:
            return std::fabs(1.0F - first) < kLeastDivisor ? 0.0F : (third * (second / (1.0F - first))) + ((1.0F - third) * second);
        case Op::Screen:
            return (third * (first + second - (first * second))) + ((1.0F - third) * second);
        case Op::Outside:
            return first * (1.0F - second);
        case Op::IfGreater:
            return first > second ? third : fourth;
        case Op::IfGreaterEq:
            return first >= second ? third : fourth;
        case Op::IfEqual:
            return first == second ? third : fourth;
        case Op::And:
            return truth(first != 0.0F && second != 0.0F);
        case Op::Or:
            return truth(first != 0.0F || second != 0.0F);
        case Op::Not:
            return truth(first == 0.0F);
        default:
            return 0.0F;
    }
}

/** Whether an operation is taken one component at a time. **/
MOYA_KERNEL inline bool componentwise(Op op) {
    return static_cast<std::uint32_t>(op) <= static_cast<std::uint32_t>(Op::Not);
}

/** A colour's hue, saturation and value, each from zero to one. **/
MOYA_KERNEL inline void hsvOf(const float * rgb, float * out) {
    const float most = std::fmax(rgb[0], std::fmax(rgb[1], rgb[2]));
    const float least = std::fmin(rgb[0], std::fmin(rgb[1], rgb[2]));
    const float delta = most - least;
    const float saturation = most > 0.0F ? delta / most : 0.0F;
    float hue = 0.0F;
    if (saturation > 0.0F) {
        if (rgb[0] >= most) {
            hue = (rgb[1] - rgb[2]) / delta;
        } else if (rgb[1] >= most) {
            hue = 2.0F + ((rgb[2] - rgb[0]) / delta);
        } else {
            hue = kBlueSector + ((rgb[0] - rgb[1]) / delta);
        }
        hue /= kHueSectors;
        if (hue < 0.0F) {
            hue += 1.0F;
        }
    }
    out[0] = hue;
    out[1] = saturation;
    out[2] = most;
}

/** The colour of a hue, saturation and value, the hue taken around the circle once. **/
MOYA_KERNEL inline void rgbOf(const float * hsv, float * out) {
    const float value = hsv[2];
    const float saturation = hsv[1];
    if (saturation < kLeastSaturation) {
        out[0] = value;
        out[1] = value;
        out[2] = value;
        return;
    }
    const float sixths = kHueSectors * (hsv[0] - std::floor(hsv[0]));
    const float sector = std::floor(sixths);
    const float within = sixths - sector;
    const float low = value * (1.0F - saturation);
    const float falling = value * (1.0F - (saturation * within));
    const float rising = value * (1.0F - (saturation * (1.0F - within)));
    const float sectors[6][3] = { { value, rising, low }, { falling, value, low }, { low, value, rising }, { low, falling, value },
        { rising, low, value }, { value, low, falling } };
    const auto chosen = static_cast<std::uint32_t>(sector) < 6U ? static_cast<std::uint32_t>(sector) : 5U;
    out[0] = sectors[chosen][0];
    out[1] = sectors[chosen][1];
    out[2] = sectors[chosen][2];
}

/** The operation an instruction names that takes its operands whole, into out. **/
MOYA_KERNEL inline void whole(const Instruction & instruction, const float * registers, float * out) {
    const float * first = registers + instruction.in[0];
    const float * second = registers + instruction.in[1];
    const std::uint32_t width = instruction.widths[0];
    switch (instruction.op) {
        case Op::Normalize:
        case Op::Magnitude: {
            float squared = 0.0F;
            for (std::uint32_t component = 0; component < width; ++component) {
                squared += first[component] * first[component];
            }
            const float length = std::sqrt(squared);
            for (std::uint32_t component = 0; component < width; ++component) {
                out[component] = length > 0.0F ? first[component] / length : 0.0F;
            }
            if (instruction.op == Op::Magnitude) {
                out[0] = length;
            }
            return;
        }
        case Op::Dot: {
            float sum = 0.0F;
            for (std::uint32_t component = 0; component < width; ++component) {
                sum += first[component] * second[component];
            }
            out[0] = sum;
            return;
        }
        case Op::Cross:
            out[0] = (first[1] * second[2]) - (first[2] * second[1]);
            out[1] = (first[2] * second[0]) - (first[0] * second[2]);
            out[2] = (first[0] * second[1]) - (first[1] * second[0]);
            return;
        case Op::Luminance: {
            const float luminance = (first[0] * second[0]) + (first[1] * second[1]) + (first[2] * second[2]);
            out[0] = luminance;
            out[1] = luminance;
            out[2] = luminance;
            out[3] = width > 3U ? first[3] : 0.0F;
            return;
        }
        case Op::RgbToHsv:
        case Op::HsvToRgb:
            if (instruction.op == Op::RgbToHsv) {
                hsvOf(first, out);
            } else {
                rgbOf(first, out);
            }
            out[3] = width > 3U ? first[3] : 0.0F;
            return;
        case Op::Extract: {
            const float index = std::fmin(std::fmax(std::floor(second[0]), 0.0F), static_cast<float>(width) - 1.0F);
            out[0] = first[static_cast<std::uint32_t>(index)];
            return;
        }
        case Op::Combine: {
            std::uint32_t at = 0;
            for (std::uint32_t i = 0; i < kMostOperands; ++i) {
                for (std::uint32_t component = 0; component < instruction.widths[i] && at < kMostComponents; ++component) {
                    out[at++] = registers[instruction.in[i] + component];
                }
            }
            return;
        }
        default:
            return;
    }
}

/** Runs one instruction over the registers. **/
MOYA_KERNEL inline void execute(const Instruction & instruction, float * registers) {
    float out[kMostComponents] {};
    if (componentwise(instruction.op)) {
        for (std::uint32_t component = 0; component < instruction.width && component < kMostComponents; ++component) {
            float operands[kMostOperands] {};
            for (std::uint32_t i = 0; i < kMostOperands; ++i) {
                operands[i] = instruction.widths[i] > 0U ? operand(registers, instruction, i, component) : 0.0F;
            }
            out[component] = componentOf(instruction.op, operands);
        }
    } else {
        whole(instruction, registers, out);
    }
    for (std::uint32_t component = 0; component < instruction.width && component < kMostComponents; ++component) {
        registers[instruction.out + component] = out[component];
    }
}

/**
 * Runs a program, from the values its registers hold before it, and gives the surface's
 * parameters its registers hold after it. The surface is not thin-walled.
 **/
MOYA_KERNEL inline surface::OpenPbr evaluate(const Array<Instruction> & instructions, const Array<float> & values, const Program & program) {
    float registers[kRegisters] {};
    for (std::uint32_t i = 0; i < program.size && i < kRegisters; ++i) {
        registers[i] = values[program.values + i];
    }
    for (std::uint32_t i = 0; i < program.count; ++i) {
        execute(instructions[program.instructions + i], registers);
    }
    return parameters(registers);
}

};  // namespace v3d::moya::kernel::program

/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/program/Instruction.h>
#include <moya/libmoya/kernel/program/Op.h>
#include <moya/libmoya/kernel/program/Parameters.h>
#include <moya/libmoya/kernel/program/Pattern.h>
#include <moya/libmoya/kernel/program/Program.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Blackbody.h>
#include <moya/libmoya/kernel/surface/Normal.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/kernel/surface/TextureFootprint.h>
#include <moya/libmoya/kernel/texture/Address.h>
#include <moya/libmoya/kernel/texture/Filtering.h>
#include <moya/libmoya/kernel/texture/Lookup.h>
#include <moya/libmoya/kernel/texture/Sampling.h>
#include <moya/libmoya/kernel/texture/Texels.h>
#include <moya/libmoya/kernel/texture/Untextured.h>

#include <cmath>
#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::kernel::program {

/** The most components a value holds: a matrix44's. **/
inline constexpr std::uint32_t kMostComponents = 16;

/** The components of a matrix33 and of a matrix44. **/
inline constexpr std::uint32_t kMatrix3 = 9;
inline constexpr std::uint32_t kMatrix4 = 16;

/** How close to zero burn's fg, and dodge's one less fg, may come before it gives zero. **/
inline constexpr float kLeastDivisor = 1.0e-8F;

/** The sixths of the circle a hue is taken in, one for each primary and secondary colour. **/
inline constexpr float kHueSectors = 6.0F;

/** Where blue's sector begins, in sixths of the circle. **/
inline constexpr float kBlueSector = 4.0F;

/** The least saturation a colour has a hue at. **/
inline constexpr float kLeastSaturation = 1.0e-4F;

/** How small bg's alpha may be before disjointover gives no colour, as MaterialX's M_FLOAT_EPS. **/
inline constexpr float kLeastAlpha = 1.0e-8F;

/** The number of address modes, by which Image's fourth operand holds v's above u's. **/
inline constexpr std::uint32_t kAddressModes = 4;

/** The share of a height's differences heighttonormal takes, as MaterialX's matches a Sobel's. **/
inline constexpr float kSobelScale = 1.0F / 16.0F;

/** The least squared length heighttonormal's normal has before it is flat: M_FLOAT_EPS squared. **/
inline constexpr float kLeastHeightNormal = 1.0e-16F;

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

/**
 * A matrix's registers, row after row, as glm holds a matrix, column after column. glm therefore
 * holds its transpose, and a product of two is the product of their transposes in the other
 * order. An inverse, a transpose and a determinant are the same either way round.
 **/
MOYA_KERNEL inline glm::mat4 loaded4(const float * registers) {
    glm::mat4 out { 1.0F };
    for (std::uint32_t i = 0; i < kMatrix4; ++i) {
        out[static_cast<glm::length_t>(i / 4U)][static_cast<glm::length_t>(i % 4U)] = registers[i];
    }
    return out;
}

MOYA_KERNEL inline glm::mat3 loaded3(const float * registers) {
    glm::mat3 out { 1.0F };
    for (std::uint32_t i = 0; i < kMatrix3; ++i) {
        out[static_cast<glm::length_t>(i / 3U)][static_cast<glm::length_t>(i % 3U)] = registers[i];
    }
    return out;
}

MOYA_KERNEL inline void stored(const glm::mat4 & matrix, float * out) {
    for (std::uint32_t i = 0; i < kMatrix4; ++i) {
        out[i] = matrix[static_cast<glm::length_t>(i / 4U)][static_cast<glm::length_t>(i % 4U)];
    }
}

MOYA_KERNEL inline void stored(const glm::mat3 & matrix, float * out) {
    for (std::uint32_t i = 0; i < kMatrix3; ++i) {
        out[i] = matrix[static_cast<glm::length_t>(i / 3U)][static_cast<glm::length_t>(i % 3U)];
    }
}

/** A matrix operation of matrix33 or matrix44 operands, by the components of the first. **/
MOYA_KERNEL inline void matrixOf(const Instruction & instruction, const float * first, const float * second, float * out) {
    const bool four = instruction.widths[0] == kMatrix4;
    switch (instruction.op) {
        case Op::Product:
            if (four) {
                stored(loaded4(second) * loaded4(first), out);
            } else {
                stored(loaded3(second) * loaded3(first), out);
            }
            return;
        case Op::Quotient:
            if (four) {
                stored(glm::inverse(loaded4(second)) * loaded4(first), out);
            } else {
                stored(glm::inverse(loaded3(second)) * loaded3(first), out);
            }
            return;
        case Op::Transpose:
            if (four) {
                stored(glm::transpose(loaded4(first)), out);
            } else {
                stored(glm::transpose(loaded3(first)), out);
            }
            return;
        case Op::Determinant:
            out[0] = four ? glm::determinant(loaded4(first)) : glm::determinant(loaded3(first));
            return;
        case Op::Inverse:
            if (four) {
                stored(glm::inverse(loaded4(first)), out);
            } else {
                stored(glm::inverse(loaded3(first)), out);
            }
            return;
        default:
            return;
    }
}

/** A vector as a row times a matrix, taken with a last component of one before a larger matrix. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the vector, then the matrix
MOYA_KERNEL inline void transformed(const Instruction & instruction, const float * vector, const float * matrix, float * out) {
    const std::uint32_t width = instruction.widths[0];
    if (instruction.widths[1] == kMatrix4) {
        const glm::vec4 row { vector[0], vector[1], width > 2U ? vector[2] : 0.0F, width > 3U ? vector[3] : 1.0F };
        const glm::vec4 turned = loaded4(matrix) * row;
        for (std::uint32_t component = 0; component < width; ++component) {
            out[component] = turned[static_cast<glm::length_t>(component)];
        }
        return;
    }
    const glm::vec3 row { vector[0], vector[1], width > 2U ? vector[2] : 1.0F };
    const glm::vec3 turned = loaded3(matrix) * row;
    for (std::uint32_t component = 0; component < width; ++component) {
        out[component] = turned[static_cast<glm::length_t>(component)];
    }
}

/** Whether an operation composites colours with alpha, which compositeOf() computes. **/
MOYA_KERNEL inline bool composited(Op op) {
    return static_cast<std::uint32_t>(op) >= static_cast<std::uint32_t>(Op::Premult) && static_cast<std::uint32_t>(op) <= static_cast<std::uint32_t>(Op::DisjointOver);
}

/** One component of over, in, mask or out, before its mix. **/
MOYA_KERNEL inline float layeredOf(Op op, const float * fg, const float * bg, std::uint32_t component) {
    switch (op) {
        case Op::Over:
            return fg[component] + (bg[component] * (1.0F - fg[3]));
        case Op::In:
            return fg[component] * bg[3];
        case Op::Mask:
            return bg[component] * fg[3];
        default:
            return fg[component] * (1.0F - bg[3]);
    }
}

/** disjointover before its mix: fg plus bg, scaled to fit where their alphas pass one. **/
MOYA_KERNEL inline void disjointOf(const float * fg, const float * bg, float * made) {
    const float summed = fg[3] + bg[3];
    const bool over = summed > 1.0F;
    const bool none = over && std::fabs(bg[3]) < kLeastAlpha;
    const float scale = over && !none ? (1.0F - fg[3]) / bg[3] : 1.0F;
    for (std::uint32_t component = 0; component < 3U; ++component) {
        made[component] = none ? 0.0F : fg[component] + (bg[component] * scale);
    }
    made[3] = std::fmin(summed, 1.0F);
}

/**
 * A compositing operation of colours with alpha, as MaterialX's OSL implementations compute it.
 * Each but premult and unpremult is mixed with bg by mix, bg where mix is zero.
 **/
MOYA_KERNEL inline void compositeOf(const Instruction & instruction, const float * registers, float * out) {
    const float * fg = registers + instruction.in[0];
    const float * bg = registers + instruction.in[1];
    const float mix = registers[instruction.in[2]];
    float made[4] {};
    switch (instruction.op) {
        case Op::Premult:
        case Op::Unpremult:
            for (std::uint32_t component = 0; component < 3U; ++component) {
                out[component] = instruction.op == Op::Premult ? fg[component] * fg[3] : fg[component] / fg[3];
            }
            out[3] = fg[3];
            return;
        case Op::Matte:
            for (std::uint32_t component = 0; component < 3U; ++component) {
                made[component] = (fg[component] * fg[3]) + (bg[component] * (1.0F - fg[3]));
            }
            made[3] = fg[3] + (bg[3] * (1.0F - fg[3]));
            break;
        case Op::DisjointOver:
            disjointOf(fg, bg, made);
            break;
        default:
            for (std::uint32_t component = 0; component < 4U; ++component) {
                made[component] = layeredOf(instruction.op, fg, bg, component);
            }
            break;
    }
    for (std::uint32_t component = 0; component < 4U; ++component) {
        out[component] = (made[component] * mix) + ((1.0F - mix) * bg[component]);
    }
}

/** Three components from a first register. **/
MOYA_KERNEL inline glm::vec3 vectorAt(const float * registers, std::uint32_t first) {
    return glm::vec3 { registers[first], registers[first + 1U], registers[first + 2U] };
}

/**
 * A normal from a value in tangent space, as MaterialX's OSL normalmap gives it. The value runs
 * from zero to one on each axis, and is taken to run from minus one to one. Its first two
 * components are scaled by the scale, of one component or two, along the tangent and the
 * bitangent, which are first laid flat against the normal and against each other. A value of no
 * length, or one that gives no direction, gives the normal.
 **/
MOYA_KERNEL inline glm::vec3 normalMapOf(const Instruction & instruction, const float * registers) {
    const glm::vec3 value = vectorAt(registers, instruction.in[0]);
    const float * scale = registers + instruction.in[1];
    const glm::vec2 scaled { scale[0], instruction.widths[1] > 1U ? scale[1] : scale[0] };
    const glm::vec3 normal = vectorAt(registers, instruction.in[2]);
    const glm::vec3 tangent = vectorAt(registers, instruction.in[3]);
    const glm::vec3 bitangent = vectorAt(registers, instruction.in[4]);
    if (value.x == 0.0F && value.y == 0.0F && value.z == 0.0F) {
        return normal;
    }
    const glm::vec3 within = (value * 2.0F) - 1.0F;
    const glm::vec3 along = surface::unitOrNone(tangent - (normal * glm::dot(tangent, normal)));
    const glm::vec3 across = surface::unitOrNone(bitangent - (normal * glm::dot(bitangent, normal)) - (along * glm::dot(bitangent, along)));
    const glm::vec3 out = surface::unitOrNone((along * (within.x * scaled.x)) + (across * (within.y * scaled.y)) + (normal * within.z));
    return glm::dot(out, out) > 0.0F ? out : normal;
}

/**
 * A value in tangent space from a height's differences, as MaterialX's heighttonormal gives it.
 * Each difference is the height a step along one axis less the height at the point, times the
 * scale and kSobelScale, beside the texture coordinates' differences along the same axis. The
 * normal is the cross product of the two steps in s, t and height, turned to face up. Steps that
 * give no direction give a flat normal. The normal, of unit length, is taken from minus one to one
 * onto zero to one.
 **/
MOYA_KERNEL inline glm::vec3 heightToNormalOf(const Instruction & instruction, const float * registers) {
    const float * heights = registers + instruction.in[0];
    const float scale = registers[instruction.in[1]] * kSobelScale;
    const float * coordinates = registers + instruction.in[2];
    const glm::vec3 first { coordinates[2] - coordinates[0], coordinates[3] - coordinates[1], (heights[1] - heights[0]) * scale };
    const glm::vec3 second { coordinates[4] - coordinates[0], coordinates[5] - coordinates[1], (heights[2] - heights[0]) * scale };
    glm::vec3 normal = glm::cross(first, second);
    if (glm::dot(normal, normal) < kLeastHeightNormal) {
        normal = glm::vec3 { 0.0F, 0.0F, 1.0F };
    } else if (normal.z < 0.0F) {
        normal = -normal;
    }
    return (glm::normalize(normal) * 0.5F) + 0.5F;
}

/**
 * A vector operation: normalize, magnitude, dotproduct, crossproduct, luminance, blackbody or a
 * hue's.
 **/
MOYA_KERNEL inline void vectorOf(const Instruction & instruction, const float * first, const float * second, float * out) {
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
        case Op::Blackbody: {
            const glm::vec3 colour = spectrum::blackbody(first[0]);
            out[0] = colour.x;
            out[1] = colour.y;
            out[2] = colour.z;
            return;
        }
        case Op::RgbToHsv:
            hsvOf(first, out);
            out[3] = width > 3U ? first[3] : 0.0F;
            return;
        case Op::HsvToRgb:
            rgbOf(first, out);
            out[3] = width > 3U ? first[3] : 0.0F;
            return;
        default:
            return;
    }
}

/** A channel operation: extract, combine, or creatematrix's rows and translation. **/
MOYA_KERNEL inline void channelsOf(const Instruction & instruction, const float * registers, float * out) {
    const float * first = registers + instruction.in[0];
    switch (instruction.op) {
        case Op::Extract: {
            // a matrix gives its row at the index, and anything else its component
            const std::uint32_t size = instruction.width;
            const std::uint32_t rows = instruction.widths[0] / size;
            const float held = std::fmin(std::fmax(std::floor(registers[instruction.in[1]]), 0.0F), static_cast<float>(rows) - 1.0F);
            const auto index = static_cast<std::uint32_t>(held);  // checked: held to [0, rows - 1] above
            for (std::uint32_t component = 0; component < size; ++component) {
                out[component] = first[(index * size) + component];
            }
            return;
        }
        case Op::Rows:
            // three rows, each with a last zero, and a translation with a last one
            for (std::uint32_t row = 0; row < 4U; ++row) {
                for (std::uint32_t column = 0; column < 3U; ++column) {
                    out[(row * 4U) + column] = registers[instruction.in[row] + column];
                }
                out[(row * 4U) + 3U] = row == 3U ? 1.0F : 0.0F;
            }
            return;
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

/** The operation an instruction names that takes its operands whole, into out. **/
MOYA_KERNEL inline void whole(const Instruction & instruction, const float * registers, float * out) {
    const float * first = registers + instruction.in[0];
    const float * second = registers + instruction.in[1];
    if (patterned(instruction.op)) {
        patternOf(instruction, registers, out);
        return;
    }
    if (composited(instruction.op)) {
        compositeOf(instruction, registers, out);
        return;
    }
    switch (instruction.op) {
        case Op::Extract:
        case Op::Rows:
        case Op::Combine:
            channelsOf(instruction, registers, out);
            return;
        case Op::Product:
        case Op::Quotient:
        case Op::Transpose:
        case Op::Determinant:
        case Op::Inverse:
            matrixOf(instruction, first, second, out);
            return;
        case Op::Transform:
            transformed(instruction, first, second, out);
            return;
        case Op::NormalMap:
        case Op::HeightToNormal: {
            const glm::vec3 made = instruction.op == Op::NormalMap ? normalMapOf(instruction, registers) : heightToNormalOf(instruction, registers);
            out[0] = made.x;
            out[1] = made.y;
            out[2] = made.z;
            return;
        }
        default:
            vectorOf(instruction, first, second, out);
            return;
    }
}

/** A direction at a shading point, in the world or turned into its geometry's own space. **/
MOYA_KERNEL inline glm::vec3 directionIn(const surface::ShadingPoint & point, const glm::vec3 & world, std::uint32_t space, bool normal) {
    if (space == kWorld) {
        return world;
    }
    const glm::mat3 turn { point.toWorld };
    // a normal moves by the inverse transpose, so it moves back into the geometry by the transpose
    const glm::vec3 own = normal ? glm::transpose(turn) * world : glm::inverse(turn) * world;
    return glm::dot(own, own) > 0.0F ? glm::normalize(own) : own;
}

/**
 * A vector at a shading point from one space into another: a position, a direction, or a normal,
 * which moves by the inverse transpose and is of unit length.
 **/
MOYA_KERNEL inline glm::vec3 between(const surface::ShadingPoint & point, Op op, const glm::vec3 & given, std::uint32_t from, std::uint32_t to) {
    if (from == to) {
        return given;
    }
    const glm::mat4 toWorld = from == kWorld ? glm::inverse(point.toWorld) : point.toWorld;
    if (op == Op::TransformPoint) {
        return glm::vec3(toWorld * glm::vec4(given, 1.0F));
    }
    if (op == Op::TransformVector) {
        return glm::mat3(toWorld) * given;
    }
    const glm::vec3 turned = glm::transpose(glm::inverse(glm::mat3(toWorld))) * given;
    return glm::dot(turned, turned) > 0.0F ? glm::normalize(turned) : turned;
}

/** An operation that reads the shading point, into out. **/
MOYA_KERNEL inline void geometric(const Instruction & instruction, const float * registers, const surface::Surfaces & surfaces,
    const surface::ShadingPoint & point, float * out) {
    const std::uint32_t chosen = instruction.in[0];
    glm::vec3 value { 0.0F };
    switch (instruction.op) {
        case Op::TransformPoint:
        case Op::TransformVector:
        case Op::TransformNormal: {
            const float * given = registers + chosen;
            value = between(point, instruction.op, glm::vec3 { given[0], given[1], given[2] }, instruction.in[1], instruction.in[2]);
            break;
        }
        case Op::Primvar:
            if (!surfaces.read(point, chosen, out, instruction.width)) {
                for (std::uint32_t component = 0; component < instruction.width && component < kMostComponents; ++component) {
                    out[component] = registers[instruction.in[1] + component];
                }
            }
            return;
        case Op::Position:
            value = chosen == kWorld ? point.position : glm::vec3(glm::inverse(point.toWorld) * glm::vec4(point.position, 1.0F));
            break;
        case Op::Normal:
            value = directionIn(point, point.shading, chosen, true);
            break;
        case Op::Tangent:
            value = directionIn(point, surfaces.tangentFrame(point).tangent, chosen, false);
            break;
        case Op::Bitangent:
            value = directionIn(point, surfaces.tangentFrame(point).bitangent, chosen, false);
            break;
        default:
            return;
    }
    out[0] = value.x;
    out[1] = value.y;
    out[2] = value.z;
}

/** An image read at an Image instruction's coordinate, over the point's footprint, into out. **/
template <texture::Texels X>
MOYA_KERNEL void imageOf(const Instruction & instruction, const float * registers, const surface::Surfaces & surfaces, const X & textures,
    const surface::ShadingPoint & point, float * out) {
    const float * coordinate = registers + instruction.in[0];
    const float * given = registers + instruction.in[1];
    glm::vec4 fallback { 0.0F };
    for (std::uint32_t component = 0; component < instruction.widths[1] && component < 4U; ++component) {
        fallback[static_cast<int>(component)] = given[component];
    }
    const texture::Sampling sampling { .s = static_cast<texture::Address>(instruction.in[3] % kAddressModes),
        .t = static_cast<texture::Address>((instruction.in[3] / kAddressModes) % kAddressModes),
        .filtering = static_cast<texture::Filtering>(instruction.in[4] % 3U) };
    const surface::TextureFootprint footprint = surfaces.textureFootprint(point);
    const glm::vec4 read = texture::lookup(textures, instruction.in[2], glm::vec2 { coordinate[0], coordinate[1] }, footprint, sampling, fallback);
    for (std::uint32_t component = 0; component < instruction.width && component < 4U; ++component) {
        out[component] = read[static_cast<int>(component)];
    }
}

/** Runs one instruction over the registers at a shading point, through a texture access. **/
template <texture::Texels X>
MOYA_KERNEL void execute(const Instruction & instruction, float * registers, const surface::Surfaces & surfaces, const X & textures,
    const surface::ShadingPoint & point) {
    float out[kMostComponents] {};
    if (instruction.op == Op::Image) {
        imageOf(instruction, registers, surfaces, textures, point, out);
    } else if (static_cast<std::uint32_t>(instruction.op) >= static_cast<std::uint32_t>(Op::Primvar)) {
        geometric(instruction, registers, surfaces, point, out);
    } else if (componentwise(instruction.op)) {
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
 * Runs a program at a shading point, from the values its registers hold before it, leaving in
 * registers what they hold after it. The surfaces hold the program's instructions and values, and
 * its images are read through a device's texture access. Each instruction after a Shift reads the
 * shading point it moved to.
 **/
template <texture::Texels X>
MOYA_KERNEL void run(const surface::Surfaces & surfaces, const X & textures, const surface::ShadingPoint & point, const Program & program, float * registers) {
    for (std::uint32_t i = 0; i < program.size && i < kRegisters; ++i) {
        registers[i] = surfaces.programValues[program.values + i];
    }
    surface::ShadingPoint at = point;
    for (std::uint32_t i = 0; i < program.count; ++i) {
        const Instruction & instruction = surfaces.instructions[program.instructions + i];
        if (instruction.op == Op::Shift) {
            at = surfaces.shifted(point, instruction.in[0]);
            continue;
        }
        execute(instruction, registers, surfaces, textures, at);
    }
}

/**
 * Runs a surface's program at a shading point, and gives the parameters its registers hold after
 * it, as run() leaves them. The surface is not thin-walled.
 **/
template <texture::Texels X>
MOYA_KERNEL surface::OpenPbr evaluate(const surface::Surfaces & surfaces, const X & textures, const surface::ShadingPoint & point, const Program & program) {
    float registers[kRegisters] {};
    const std::uint64_t mark = texture::started(textures);
    run(surfaces, textures, point, program, registers);
    texture::finished(textures, texture::Work::Program, mark);
    return parameters(registers);
}

/** A program run with no textures, so each image gives its default. **/
MOYA_KERNEL inline surface::OpenPbr evaluate(const surface::Surfaces & surfaces, const surface::ShadingPoint & point, const Program & program) {
    return evaluate(surfaces, texture::Untextured {}, point, program);
}

};  // namespace v3d::moya::kernel::program

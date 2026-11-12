/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::kernel::program {

/**
 * What an instruction computes, each as the MaterialX node of the same name defines it. Each
 * operation up to Not is taken one component at a time, over the components of its output, and
 * an operand of one component is spread over all of them. The rest take their operands whole.
 * Integers and booleans are held as floats, a boolean as one or zero.
 **/
enum class Op : std::uint8_t {
    Copy,        /**< the first operand: dot, constant and convert **/
    Add,         /**< in1 + in2 **/
    Subtract,    /**< in1 - in2, and invert as amount - in **/
    Multiply,    /**< in1 * in2, and inside as in * mask **/
    Divide,      /**< in1 / in2 **/
    Modulo,      /**< in1 less in2 times the floor of in1 / in2 **/
    Fract,       /**< in less its floor **/
    Abs,
    Floor,
    Ceil,
    Round,       /**< to the nearest whole number, halves away from zero **/
    Power,       /**< in1 to the power in2 **/
    Sin,
    Cos,
    Tan,
    Asin,
    Acos,
    Atan2,       /**< the angle of (inx, iny), from iny then inx **/
    Sqrt,
    Ln,
    Exp,
    Sign,        /**< -1, 0 or 1 **/
    Clamp,       /**< in held to [low, high] **/
    Min,
    Max,
    Remap,       /**< in from [inlow, inhigh] onto [outlow, outhigh] **/
    Smoothstep,  /**< zero at low and below, one at high and above, and Hermite's between **/
    Mix,         /**< bg + (fg - bg) * mix, from fg, bg, then mix **/
    Plus,        /**< fg + bg, mixed over bg by mix **/
    Minus,       /**< bg - fg, mixed over bg by mix **/
    Difference,  /**< |bg - fg|, mixed over bg by mix **/
    Burn,        /**< 1 - (1 - bg) / fg, mixed over bg by mix, and zero where fg is **/
    Dodge,       /**< bg / (1 - fg), mixed over bg by mix, and zero where fg is one **/
    Screen,      /**< fg + bg - fg bg, mixed over bg by mix **/
    Outside,     /**< in * (1 - mask) **/
    IfGreater,   /**< in1 where value1 > value2, and in2 where it is not **/
    IfGreaterEq, /**< in1 where value1 >= value2, and in2 where it is not **/
    IfEqual,     /**< in1 where value1 == value2, and in2 where it is not **/
    And,
    Or,
    Not,
    Normalize,   /**< in over its length, or zero where it has none **/
    Magnitude,   /**< in's length **/
    Dot,         /**< in1's dot product with in2 **/
    Cross,       /**< in1's cross product with in2 **/
    Luminance,   /**< in's dot product with lumacoeffs in each of its colour's components **/
    RgbToHsv,
    HsvToRgb,
    Extract,     /**< in's component at index, held to its components **/
    Combine      /**< each operand's components, one operand after another **/
};

};  // namespace v3d::moya::kernel::program

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
 * Integers and booleans are held as floats, a boolean as one or zero. A matrix is held row after
 * row, as MaterialX writes it, and a vector is transformed as a row on the matrix's left, so a
 * translation is the last row.
 *
 * The operations from Primvar on read the shading point. The first operand of Primvar to
 * Bitangent is not a register but a number: a primvar's, or kObject or kWorld for the space a
 * direction or a position is given in. The transforms read their vector from the first operand,
 * and take the spaces they turn it from and to as their second and third.
 *
 * Image reads a texture at a coordinate, its first operand, and gives its second where it reads
 * nothing. Its third operand is the image's number in the catalog, its fourth u's address mode
 * and four times v's, and its fifth how it filters, each as a number in the order MaterialX's
 * enumerations give.
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
    Extract,     /**< in's component at index, or a matrix's row, held to those it has **/
    Combine,     /**< each operand's components, one operand after another **/
    Product,     /**< the matrix product of in1 and in2 **/
    Quotient,    /**< in1 times the inverse of in2 **/
    Transpose,
    Determinant,
    Inverse,
    Transform,   /**< in as a row times mat, a vector2 or vector3 taken with a last one before a larger matrix **/
    Rows,        /**< a matrix44 of three rows from in1 to in3 and a translation from in4 **/
    Noise,       /**< Perlin noise times amplitude plus pivot, from amplitude, pivot, then a position of two or three **/
    Fractal,     /**< fractal noise times amplitude, from amplitude, octaves, lacunarity, diminish, then a position **/
    CellNoise,   /**< from zero to one for the unit cell a position of two or three lies in **/
    Worley,      /**< Worley noise from a position, jitter, then style: distances when it is zero, and cell noise when one **/
    RampLeftRight,  /**< valuel to valuer as u goes from zero to one, from valuel, valuer, then texcoord **/
    RampTopBottom,  /**< valueb to valuet as v goes from zero to one, from valuet, valueb, then texcoord **/
    SplitLeftRight, /**< valuel where u is below center, and valuer where it is not, from valuel, valuer, center, then texcoord **/
    SplitTopBottom, /**< valueb where v is below center, and valuet where it is not, from valuet, valueb, center, then texcoord **/
    Rotate2d,    /**< in turned clockwise about the origin by amount in degrees, with v upward **/
    Rotate3d,    /**< in turned about axis by amount in degrees, clockwise as seen from the axis's tip **/
    Premult,     /**< in's colour times its alpha, and its alpha **/
    Unpremult,   /**< in's colour over its alpha, and its alpha **/
    Over,        /**< fg plus bg times one less fg's alpha, from fg, bg, then mix **/
    In,          /**< fg times bg's alpha, from fg, bg, then mix **/
    Mask,        /**< bg times fg's alpha, from fg, bg, then mix **/
    Matte,       /**< fg's colour by its alpha over bg's, and fg's alpha over bg's, from fg, bg, then mix **/
    Out,         /**< fg times one less bg's alpha, from fg, bg, then mix **/
    DisjointOver,  /**< fg and bg added, bg scaled to fill what fg's alpha leaves where their alphas pass one, from fg, bg, then mix **/
    Primvar,     /**< the primvar the first operand numbers, or the second operand where there is none **/
    Position,    /**< the shading point's position **/
    Normal,      /**< its shading normal **/
    Tangent,     /**< its tangent frame's tangent **/
    Bitangent,   /**< its tangent frame's bitangent **/
    TransformPoint,  /**< a position from one space to another **/
    TransformVector, /**< a direction from one space to another **/
    TransformNormal, /**< a normal from one space to another, of unit length **/
    Image        /**< an image's texels at a coordinate, filtered over the shading point's footprint **/
};

/** The space a geometric operation gives its value in: the geometry's own, or the world. **/
inline constexpr std::uint32_t kObject = 0;
inline constexpr std::uint32_t kWorld = 1;

};  // namespace v3d::moya::kernel::program

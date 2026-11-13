/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/material/internal/Operation.h>

#include <moya/libmoya/kernel/program/Op.h>

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace v3d::moya::material::internal {

std::optional<Operation> Operation::of(const std::string & category) {
    using kernel::program::Op;
    using Inputs = std::vector<std::string>;
    static const Inputs one { "in" };
    static const Inputs two { "in1", "in2" };
    static const Inputs composite { "fg", "bg", "mix" };
    static const Inputs conditional { "value1", "value2", "in1", "in2" };
    static const std::map<std::string, Operation> operations {
        { "constant", { Op::Copy, { "value" } } },
        { "premult", { Op::Premult, one } },
        { "unpremult", { Op::Unpremult, one } },
        { "over", { Op::Over, composite } },
        { "in", { Op::In, composite } },
        { "mask", { Op::Mask, composite } },
        { "matte", { Op::Matte, composite } },
        { "out", { Op::Out, composite } },
        { "disjointover", { Op::DisjointOver, composite } },
        { "dot", { Op::Copy, one } },
        { "convert", { Op::Copy, one } },
        { "add", { Op::Add, two } },
        { "subtract", { Op::Subtract, two } },
        { "multiply", { Op::Multiply, two } },
        { "divide", { Op::Divide, two } },
        { "modulo", { Op::Modulo, two } },
        { "fract", { Op::Fract, one } },
        { "invert", { Op::Subtract, { "amount", "in" } } },
        { "absval", { Op::Abs, one } },
        { "floor", { Op::Floor, one } },
        { "ceil", { Op::Ceil, one } },
        { "round", { Op::Round, one } },
        { "power", { Op::Power, two } },
        { "sin", { Op::Sin, one } },
        { "cos", { Op::Cos, one } },
        { "tan", { Op::Tan, one } },
        { "asin", { Op::Asin, one } },
        { "acos", { Op::Acos, one } },
        { "atan2", { Op::Atan2, { "iny", "inx" } } },
        { "sqrt", { Op::Sqrt, one } },
        { "ln", { Op::Ln, one } },
        { "exp", { Op::Exp, one } },
        { "sign", { Op::Sign, one } },
        { "clamp", { Op::Clamp, { "in", "low", "high" } } },
        { "min", { Op::Min, two } },
        { "max", { Op::Max, two } },
        { "remap", { Op::Remap, { "in", "inlow", "inhigh", "outlow", "outhigh" } } },
        { "smoothstep", { Op::Smoothstep, { "in", "low", "high" } } },
        { "mix", { Op::Mix, composite } },
        { "plus", { Op::Plus, composite } },
        { "minus", { Op::Minus, composite } },
        { "difference", { Op::Difference, composite } },
        { "burn", { Op::Burn, composite } },
        { "dodge", { Op::Dodge, composite } },
        { "screen", { Op::Screen, composite } },
        { "inside", { Op::Multiply, { "in", "mask" } } },
        { "outside", { Op::Outside, { "in", "mask" } } },
        { "ifgreater", { Op::IfGreater, conditional } },
        { "ifgreatereq", { Op::IfGreaterEq, conditional } },
        { "ifequal", { Op::IfEqual, conditional } },
        { "and", { Op::And, two } },
        { "or", { Op::Or, two } },
        { "not", { Op::Not, one } },
        { "normalize", { Op::Normalize, one } },
        { "magnitude", { Op::Magnitude, one } },
        { "dotproduct", { Op::Dot, two } },
        { "crossproduct", { Op::Cross, two } },
        { "luminance", { Op::Luminance, { "in", "lumacoeffs" } } },
        { "rgbtohsv", { Op::RgbToHsv, one } },
        { "hsvtorgb", { Op::HsvToRgb, one } },
        { "extract", { Op::Extract, { "in", "index" } } },
        { "combine2", { Op::Combine, two } },
        { "combine3", { Op::Combine, { "in1", "in2", "in3" } } },
        { "combine4", { Op::Combine, { "in1", "in2", "in3", "in4" } } },
        { "transpose", { Op::Transpose, one } },
        { "determinant", { Op::Determinant, one } },
        { "invertmatrix", { Op::Inverse, one } },
        { "transformmatrix", { Op::Transform, { "in", "mat" } } },
        { "noise2d", { Op::Noise, { "amplitude", "pivot", "texcoord" } } },
        { "noise3d", { Op::Noise, { "amplitude", "pivot", "position" } } },
        { "fractal2d", { Op::Fractal, { "amplitude", "octaves", "lacunarity", "diminish", "texcoord" } } },
        { "fractal3d", { Op::Fractal, { "amplitude", "octaves", "lacunarity", "diminish", "position" } } },
        { "cellnoise2d", { Op::CellNoise, { "texcoord" } } },
        { "cellnoise3d", { Op::CellNoise, { "position" } } },
        { "worleynoise2d", { Op::Worley, { "texcoord", "jitter", "style" } } },
        { "worleynoise3d", { Op::Worley, { "position", "jitter", "style" } } },
        { "ramplr", { Op::RampLeftRight, { "valuel", "valuer", "texcoord" } } },
        { "ramptb", { Op::RampTopBottom, { "valuet", "valueb", "texcoord" } } },
        { "splitlr", { Op::SplitLeftRight, { "valuel", "valuer", "center", "texcoord" } } },
        { "splittb", { Op::SplitTopBottom, { "valuet", "valueb", "center", "texcoord" } } },
        { "rotate2d", { Op::Rotate2d, { "in", "amount" } } },
        { "rotate3d", { Op::Rotate3d, { "in", "amount", "axis" } } },
        { "normalmap", { Op::NormalMap, { "in", "scale", "normal", "tangent", "bitangent" } } },
    };
    const auto found = operations.find(category);
    if (found == operations.end()) {
        return std::nullopt;
    }
    return found->second;
}

};  // namespace v3d::moya::material::internal

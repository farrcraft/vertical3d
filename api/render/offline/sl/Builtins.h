/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/syntax/Function.h>

#include <cstdint>
#include <string>
#include <vector>

#include "Types.h"

namespace v3d::render::offline::sl {

/**
 * One way a standard library function may be called: what it takes and what it gives back.
 *
 * This is the **declared interface**, not a statement of how the function is implemented.
 * Most of these run in the machine. `diffuse`, `specular` and `phong` are ordinary shader
 * functions written over `illuminance`, as the standard defines them. Neither a caller nor
 * the type checker can tell the difference.
 **/
class Signature final {
 public:
    /**
     * What runs a call, named where the signature is declared so that a function cannot be
     * declared without saying what it does.
     **/
    enum class Body : std::uint8_t {
        /**
         * Written in the shading language, in sources(), and inlined by the compiler. A call
         * never reaches the machine.
         **/
        SOURCE,
        /**
         * Declared but not implemented: it returns its default, and the machine reports the
         * call once. Without the report, a scene the renderer did not understand would look
         * the same as a scene that rendered nothing.
         **/
        STUB,
        // each component of the result is this function of the same component of each
        // argument, so abs() of a colour is the three absolute values. The trigonometry and
        // the angle conversions are declared for floats only
        ABS, SIGN, FLOOR, CEIL, ROUND, SQRT, EXP, LOG, RADIANS, DEGREES,
        SIN, COS, TAN, ASIN, ACOS, ATAN, MOD, POW, MIN, MAX, CLAMP, MIX, STEP, SMOOTHSTEP,
        // a triple read as a direction, not as three numbers
        LENGTH, DISTANCE, NORMALIZE, FACEFORWARD, REFLECT, REFRACT, FRESNEL,
        // one component of one value, named or indexed
        XCOMP, YCOMP, ZCOMP, SETXCOMP, SETYCOMP, SETZCOMP, COMP, SETCOMP,
        // a named coordinate space, which the renderer supplies
        PTRANSFORM, VTRANSFORM, NTRANSFORM, CTRANSFORM, MTRANSFORM, DEPTH,
        // a matrix
        DETERMINANT, TRANSLATE, ROTATE, SCALE,
        // a pattern, out of an image or out of nothing
        TEXTURE, NOISE,
        // computed from the renderer's lights, shadows and rays
        AMBIENT, TRANSMISSION, TRACE,
        PRINTF
    };

    /**
     * What a parameter accepts. The last three are the families: a function that takes any
     * position or direction, one that takes any number-like value, and printf's tail.
     **/
    enum class Argument : std::uint8_t {
        FLOAT,
        POINT,
        VECTOR,
        NORMAL,
        COLOR,
        MATRIX,
        STRING,
        /** point, vector or normal. **/
        POINTLIKE,
        /** float, or any of the four three-float types. **/
        NUMBER,
        /** Anything at all. **/
        ANY
    };

    std::string name;
    Body body = Body::STUB;
    /** The result type, unless resultFrom or promotes says where else to take it from. **/
    Type result = Type::FLOAT;
    /** The argument whose type the result takes, or -1 for the fixed one above. **/
    int resultFrom = -1;
    /**
     * The result is the type the arguments promote to, as an arithmetic operator's result is.
     * `max(0, Ci)` is a colour, and `max(Cs, P)` is rejected as `Cs + P` is.
     **/
    bool promotes = false;
    /**
     * Varying however uniform its arguments are, because it reads the shading point.
     * `ambient()` takes nothing and returns something different at every point on a grid.
     **/
    bool varying = false;
    /** Takes any number of further arguments after those listed - printf, and only it. **/
    bool variadic = false;
    /**
     * The first argument the function writes rather than reads, or -1. Every argument from
     * it on is written, so each has to be a variable: `fresnel` writes four of them.
     **/
    int outputs = -1;
    /**
     * The argument the function changes in place, or -1. It is read and then written, so it
     * has to be a variable the shader can assign: `setxcomp` changes one component of it.
     **/
    int updates = -1;
    std::vector<Argument> arguments;
};

/**
 * Every way the standard library can be called, by name.
 *
 * A name may appear more than once: `faceforward` takes two arguments or three, and `noise`
 * takes a float, a point or a pair. The compiler tries each in turn and takes the first that
 * accepts the call, unless the call is the operand of a cast and a later one returns the
 * cast's type. `color noise(P)` is therefore a colour of noise rather than a grey one. SL
 * chooses this way between functions that differ only in their return type.
 **/
const std::vector<Signature> & builtins();

/**
 * The signatures declared under that name, or an empty list when the name is not one.
 **/
std::vector<Signature> builtin(const std::string & name);

/**
 * The library's own functions, written in the shading language instead of C++.
 *
 * `diffuse`, `specular` and `phong` are ordinary functions over `illuminance`, as the
 * standard defines them. They have no special access to the lights, so a test can write the
 * same few lines and compare. A shader that calls one has it added to its own function list
 * and inlined from there. Nothing after the compiler treats it differently.
 *
 * A shader's own function of the same name takes precedence, so a scene can override one.
 *
 * Parsed again on every call. The compiler annotates a syntax tree in place, so each shader
 * that calls `diffuse` needs its own copy of the tree.
 **/
std::vector<syntax::Function> sources();

};  // namespace v3d::render::offline::sl

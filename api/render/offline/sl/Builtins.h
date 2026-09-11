/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "Syntax.h"

namespace v3d::render::offline::sl {

/**
 * One way a standard library function may be called: what it takes and what it gives back.
 *
 * This is the **declared interface** rather than a claim about how the function is
 * implemented. Some of these are arithmetic the machine does; `ambient`, `diffuse`,
 * `specular` and `phong` are ordinary shader functions written over `illuminance`, which is
 * both what the standard says and what makes them testable. A caller cannot tell the
 * difference, and neither can the type checker.
 **/
class Signature final {
 public:
    /**
     * What a parameter accepts. The last three are the families: a function that takes any
     * position or direction, one that takes any number-like value, and printf's tail.
     **/
    enum class Argument {
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
    /** The result type, unless resultFrom names an argument to take it from. **/
    Type result = Type::FLOAT;
    /** The argument whose type the result takes, or -1 for the fixed one above. **/
    int resultFrom = -1;
    /**
     * Varying however uniform its arguments are, because it reads the shading point.
     * `ambient()` takes nothing and answers something different at every point on a grid.
     **/
    bool varying = false;
    /** Takes any number of further arguments after those listed - printf, and only it. **/
    bool variadic = false;
    std::vector<Argument> arguments;
};

/**
 * Every way the standard library can be called, by name.
 *
 * A name may appear more than once: `faceforward` takes two arguments or three, and `noise`
 * takes a float, a point or a pair. The compiler tries each in turn and takes the first that
 * accepts the call.
 **/
const std::vector<Signature> & builtins();

/**
 * The signatures declared under that name, or an empty list when the name is not one.
 **/
std::vector<Signature> builtin(const std::string & name);

/**
 * The library's own functions, written in the language rather than in C++.
 *
 * `diffuse`, `specular` and `phong` are ordinary functions over `illuminance` rather than
 * built-ins with privileged access to the lights - which is both what the standard says
 * and what makes them testable, since a case can write the same three lines and compare.
 * A shader that calls one has it adopted into its own function list and inlined from
 * there, so nothing downstream of the compiler knows the difference.
 *
 * A shader's own function of the same name wins, which is how a scene overrides one.
 *
 * Read fresh every call rather than held: the compiler annotates a tree in place, so two
 * shaders that both call `diffuse` need two trees rather than one they take turns writing.
 **/
std::vector<Function> sources();

/**
 * Whether the built-in is declared but does nothing yet - texture, shadow and noise, each of
 * which returns its default and is reported once.
 *
 * A scene that rendered nothing and a scene that was not understood look identical from
 * outside, which is why a stub is loud rather than silent.
 **/
bool stubbed(const std::string & name);

};  // namespace v3d::render::offline::sl

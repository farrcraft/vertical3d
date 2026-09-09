/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Builtins.h"

#include <algorithm>
#include <string>
#include <vector>

namespace v3d::render::offline::sl {

namespace {

typedef Signature::Argument Argument;

Signature declare(const char* name, Type result, const std::vector<Argument> & arguments) {
    Signature signature;
    signature.name = name;
    signature.result = result;
    signature.arguments = arguments;
    return signature;
}

/**
 * A function whose result is the type of one of its arguments: normalize() of a normal is a
 * normal, and mix() of two colours is a colour.
 **/
Signature same(const char* name, int argument, const std::vector<Argument> & arguments) {
    Signature signature;
    signature.name = name;
    signature.resultFrom = argument;
    signature.arguments = arguments;
    return signature;
}

Signature shading(const char* name, Type result, const std::vector<Argument> & arguments) {
    Signature signature = declare(name, result, arguments);
    signature.varying = true;
    return signature;
}

std::vector<Signature> build() {
    std::vector<Signature> table;

    // maths, which is arithmetic over the value model
    const char* const unary[] = {
        "abs", "sign", "floor", "ceil", "round", "sqrt", "exp", "log",
        "radians", "degrees", "sin", "cos", "tan", "asin", "acos", "atan"
    };
    for (const char* const name : unary) {
        table.push_back(declare(name, Type::FLOAT, { Argument::FLOAT }));
    }
    table.push_back(declare("atan", Type::FLOAT, { Argument::FLOAT, Argument::FLOAT }));
    table.push_back(declare("log", Type::FLOAT, { Argument::FLOAT, Argument::FLOAT }));
    table.push_back(declare("mod", Type::FLOAT, { Argument::FLOAT, Argument::FLOAT }));
    table.push_back(declare("pow", Type::FLOAT, { Argument::FLOAT, Argument::FLOAT }));
    table.push_back(same("min", 0, { Argument::NUMBER, Argument::NUMBER }));
    table.push_back(same("max", 0, { Argument::NUMBER, Argument::NUMBER }));
    table.push_back(same("clamp", 0, { Argument::NUMBER, Argument::NUMBER, Argument::NUMBER }));
    table.push_back(same("mix", 0, { Argument::NUMBER, Argument::NUMBER, Argument::FLOAT }));
    table.push_back(declare("step", Type::FLOAT, { Argument::FLOAT, Argument::FLOAT }));
    table.push_back(declare("smoothstep", Type::FLOAT,
        { Argument::FLOAT, Argument::FLOAT, Argument::FLOAT }));

    // geometry
    table.push_back(declare("length", Type::FLOAT, { Argument::POINTLIKE }));
    table.push_back(declare("distance", Type::FLOAT, { Argument::POINT, Argument::POINT }));
    table.push_back(same("normalize", 0, { Argument::POINTLIKE }));
    table.push_back(same("faceforward", 0, { Argument::POINTLIKE, Argument::POINTLIKE }));
    table.push_back(same("faceforward", 0,
        { Argument::POINTLIKE, Argument::POINTLIKE, Argument::POINTLIKE }));
    table.push_back(declare("reflect", Type::VECTOR, { Argument::POINTLIKE, Argument::POINTLIKE }));
    table.push_back(declare("refract", Type::VECTOR,
        { Argument::POINTLIKE, Argument::POINTLIKE, Argument::FLOAT }));
    table.push_back(declare("depth", Type::FLOAT, { Argument::POINT }));
    table.push_back(shading("calculatenormal", Type::NORMAL, { Argument::POINT }));

    // the component accessors and their setters
    const char* const readers[] = { "xcomp", "ycomp", "zcomp" };
    for (const char* const name : readers) {
        table.push_back(declare(name, Type::FLOAT, { Argument::POINTLIKE }));
    }
    const char* const writers[] = { "setxcomp", "setycomp", "setzcomp" };
    for (const char* const name : writers) {
        table.push_back(declare(name, Type::VOID, { Argument::POINTLIKE, Argument::FLOAT }));
    }
    table.push_back(declare("comp", Type::FLOAT, { Argument::NUMBER, Argument::FLOAT }));
    table.push_back(declare("setcomp", Type::VOID,
        { Argument::NUMBER, Argument::FLOAT, Argument::FLOAT }));

    // the transforms, which are what tells the three point-like types apart
    table.push_back(declare("ptransform", Type::POINT, { Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("ptransform", Type::POINT,
        { Argument::STRING, Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("vtransform", Type::VECTOR, { Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("vtransform", Type::VECTOR,
        { Argument::STRING, Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("ntransform", Type::NORMAL, { Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("ntransform", Type::NORMAL,
        { Argument::STRING, Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("ctransform", Type::COLOR, { Argument::STRING, Argument::COLOR }));
    table.push_back(declare("ctransform", Type::COLOR,
        { Argument::STRING, Argument::STRING, Argument::COLOR }));
    table.push_back(declare("mtransform", Type::MATRIX, { Argument::STRING, Argument::MATRIX }));

    // matrix
    table.push_back(declare("determinant", Type::FLOAT, { Argument::MATRIX }));
    table.push_back(declare("translate", Type::MATRIX, { Argument::MATRIX, Argument::POINTLIKE }));
    table.push_back(declare("rotate", Type::MATRIX,
        { Argument::MATRIX, Argument::FLOAT, Argument::POINTLIKE }));
    table.push_back(declare("scale", Type::MATRIX, { Argument::MATRIX, Argument::POINTLIKE }));

    // the light model, written over illuminance rather than given privileged access
    table.push_back(shading("ambient", Type::COLOR, {}));
    table.push_back(shading("diffuse", Type::COLOR, { Argument::POINTLIKE }));
    table.push_back(shading("specular", Type::COLOR,
        { Argument::POINTLIKE, Argument::POINTLIKE, Argument::FLOAT }));
    table.push_back(shading("phong", Type::COLOR,
        { Argument::POINTLIKE, Argument::POINTLIKE, Argument::FLOAT }));
    table.push_back(declare("specularbrdf", Type::COLOR,
        { Argument::POINTLIKE, Argument::POINTLIKE, Argument::POINTLIKE, Argument::FLOAT }));

    // what the renderer answers rather than the machine: a shadow, and the phase 6 hook
    table.push_back(shading("transmission", Type::COLOR, { Argument::POINT, Argument::POINT }));
    table.push_back(shading("trace", Type::COLOR, { Argument::POINT, Argument::POINTLIKE }));

    // declared, stubbed and reported once - see stubbed()
    table.push_back(shading("texture", Type::COLOR, { Argument::STRING }));
    table.push_back(shading("texture", Type::COLOR,
        { Argument::STRING, Argument::FLOAT, Argument::FLOAT }));
    table.push_back(shading("shadow", Type::FLOAT, { Argument::STRING, Argument::POINT }));
    table.push_back(declare("noise", Type::FLOAT, { Argument::FLOAT }));
    table.push_back(declare("noise", Type::FLOAT, { Argument::FLOAT, Argument::FLOAT }));
    table.push_back(declare("noise", Type::FLOAT, { Argument::POINT }));

    // and the one anybody actually debugs with
    Signature print = declare("printf", Type::VOID, { Argument::STRING });
    print.variadic = true;
    table.push_back(print);

    return table;
}

};  // namespace

const std::vector<Signature> & builtins() {
    static const std::vector<Signature> table = build();
    return table;
}

std::vector<Signature> builtin(const std::string & name) {
    std::vector<Signature> found;
    for (const Signature & signature : builtins()) {
        if (signature.name == name) {
            found.push_back(signature);
        }
    }
    return found;
}

bool stubbed(const std::string & name) {
    return name == "texture" || name == "shadow" || name == "noise";
}

};  // namespace v3d::render::offline::sl

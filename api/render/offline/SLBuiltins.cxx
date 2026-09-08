/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SLBuiltins.h"

#include <algorithm>
#include <string>
#include <vector>

namespace v3d::render::offline {

namespace {

typedef SLSignature::Argument Argument;

SLSignature declare(const char* name, SLType result, const std::vector<Argument> & arguments) {
    SLSignature signature;
    signature.name = name;
    signature.result = result;
    signature.arguments = arguments;
    return signature;
}

/**
 * A function whose result is the type of one of its arguments: normalize() of a normal is a
 * normal, and mix() of two colours is a colour.
 **/
SLSignature same(const char* name, int argument, const std::vector<Argument> & arguments) {
    SLSignature signature;
    signature.name = name;
    signature.resultFrom = argument;
    signature.arguments = arguments;
    return signature;
}

SLSignature shading(const char* name, SLType result, const std::vector<Argument> & arguments) {
    SLSignature signature = declare(name, result, arguments);
    signature.varying = true;
    return signature;
}

std::vector<SLSignature> build() {
    std::vector<SLSignature> table;

    // maths, which is arithmetic over the value model
    const char* const unary[] = {
        "abs", "sign", "floor", "ceil", "round", "sqrt", "exp", "log",
        "radians", "degrees", "sin", "cos", "tan", "asin", "acos", "atan"
    };
    for (const char* const name : unary) {
        table.push_back(declare(name, SLType::FLOAT, { Argument::FLOAT }));
    }
    table.push_back(declare("atan", SLType::FLOAT, { Argument::FLOAT, Argument::FLOAT }));
    table.push_back(declare("log", SLType::FLOAT, { Argument::FLOAT, Argument::FLOAT }));
    table.push_back(declare("mod", SLType::FLOAT, { Argument::FLOAT, Argument::FLOAT }));
    table.push_back(declare("pow", SLType::FLOAT, { Argument::FLOAT, Argument::FLOAT }));
    table.push_back(same("min", 0, { Argument::NUMBER, Argument::NUMBER }));
    table.push_back(same("max", 0, { Argument::NUMBER, Argument::NUMBER }));
    table.push_back(same("clamp", 0, { Argument::NUMBER, Argument::NUMBER, Argument::NUMBER }));
    table.push_back(same("mix", 0, { Argument::NUMBER, Argument::NUMBER, Argument::FLOAT }));
    table.push_back(declare("step", SLType::FLOAT, { Argument::FLOAT, Argument::FLOAT }));
    table.push_back(declare("smoothstep", SLType::FLOAT,
        { Argument::FLOAT, Argument::FLOAT, Argument::FLOAT }));

    // geometry
    table.push_back(declare("length", SLType::FLOAT, { Argument::POINTLIKE }));
    table.push_back(declare("distance", SLType::FLOAT, { Argument::POINT, Argument::POINT }));
    table.push_back(same("normalize", 0, { Argument::POINTLIKE }));
    table.push_back(same("faceforward", 0, { Argument::POINTLIKE, Argument::POINTLIKE }));
    table.push_back(same("faceforward", 0,
        { Argument::POINTLIKE, Argument::POINTLIKE, Argument::POINTLIKE }));
    table.push_back(declare("reflect", SLType::VECTOR, { Argument::POINTLIKE, Argument::POINTLIKE }));
    table.push_back(declare("refract", SLType::VECTOR,
        { Argument::POINTLIKE, Argument::POINTLIKE, Argument::FLOAT }));
    table.push_back(declare("depth", SLType::FLOAT, { Argument::POINT }));
    table.push_back(shading("calculatenormal", SLType::NORMAL, { Argument::POINT }));

    // the component accessors and their setters
    const char* const readers[] = { "xcomp", "ycomp", "zcomp" };
    for (const char* const name : readers) {
        table.push_back(declare(name, SLType::FLOAT, { Argument::POINTLIKE }));
    }
    const char* const writers[] = { "setxcomp", "setycomp", "setzcomp" };
    for (const char* const name : writers) {
        table.push_back(declare(name, SLType::VOID, { Argument::POINTLIKE, Argument::FLOAT }));
    }
    table.push_back(declare("comp", SLType::FLOAT, { Argument::NUMBER, Argument::FLOAT }));
    table.push_back(declare("setcomp", SLType::VOID,
        { Argument::NUMBER, Argument::FLOAT, Argument::FLOAT }));

    // the transforms, which are what tells the three point-like types apart
    table.push_back(declare("ptransform", SLType::POINT, { Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("ptransform", SLType::POINT,
        { Argument::STRING, Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("vtransform", SLType::VECTOR, { Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("vtransform", SLType::VECTOR,
        { Argument::STRING, Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("ntransform", SLType::NORMAL, { Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("ntransform", SLType::NORMAL,
        { Argument::STRING, Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("ctransform", SLType::COLOR, { Argument::STRING, Argument::COLOR }));
    table.push_back(declare("ctransform", SLType::COLOR,
        { Argument::STRING, Argument::STRING, Argument::COLOR }));
    table.push_back(declare("mtransform", SLType::MATRIX, { Argument::STRING, Argument::MATRIX }));

    // matrix
    table.push_back(declare("determinant", SLType::FLOAT, { Argument::MATRIX }));
    table.push_back(declare("translate", SLType::MATRIX, { Argument::MATRIX, Argument::POINTLIKE }));
    table.push_back(declare("rotate", SLType::MATRIX,
        { Argument::MATRIX, Argument::FLOAT, Argument::POINTLIKE }));
    table.push_back(declare("scale", SLType::MATRIX, { Argument::MATRIX, Argument::POINTLIKE }));

    // the light model, written over illuminance rather than given privileged access
    table.push_back(shading("ambient", SLType::COLOR, {}));
    table.push_back(shading("diffuse", SLType::COLOR, { Argument::POINTLIKE }));
    table.push_back(shading("specular", SLType::COLOR,
        { Argument::POINTLIKE, Argument::POINTLIKE, Argument::FLOAT }));
    table.push_back(shading("phong", SLType::COLOR,
        { Argument::POINTLIKE, Argument::POINTLIKE, Argument::FLOAT }));
    table.push_back(declare("specularbrdf", SLType::COLOR,
        { Argument::POINTLIKE, Argument::POINTLIKE, Argument::POINTLIKE, Argument::FLOAT }));

    // what the renderer answers rather than the machine: a shadow, and the phase 6 hook
    table.push_back(shading("transmission", SLType::COLOR, { Argument::POINT, Argument::POINT }));
    table.push_back(shading("trace", SLType::COLOR, { Argument::POINT, Argument::POINTLIKE }));

    // declared, stubbed and reported once - see stubbed()
    table.push_back(shading("texture", SLType::COLOR, { Argument::STRING }));
    table.push_back(shading("texture", SLType::COLOR,
        { Argument::STRING, Argument::FLOAT, Argument::FLOAT }));
    table.push_back(shading("shadow", SLType::FLOAT, { Argument::STRING, Argument::POINT }));
    table.push_back(declare("noise", SLType::FLOAT, { Argument::FLOAT }));
    table.push_back(declare("noise", SLType::FLOAT, { Argument::FLOAT, Argument::FLOAT }));
    table.push_back(declare("noise", SLType::FLOAT, { Argument::POINT }));

    // and the one anybody actually debugs with
    SLSignature print = declare("printf", SLType::VOID, { Argument::STRING });
    print.variadic = true;
    table.push_back(print);

    return table;
}

};  // namespace

const std::vector<SLSignature> & builtins() {
    static const std::vector<SLSignature> table = build();
    return table;
}

std::vector<SLSignature> builtin(const std::string & name) {
    std::vector<SLSignature> found;
    for (const SLSignature & signature : builtins()) {
        if (signature.name == name) {
            found.push_back(signature);
        }
    }
    return found;
}

bool stubbed(const std::string & name) {
    return name == "texture" || name == "shadow" || name == "noise";
}

};  // namespace v3d::render::offline

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Builtins.h"

#include <api/render/offline/sl/syntax/Function.h>
#include <api/render/offline/sl/syntax/Shader.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "Parser.h"

namespace v3d::render::offline::sl {

namespace {

typedef Signature::Argument Argument;
typedef Signature::Body Body;

Signature declare(const char* name, Body body, Type result, const std::vector<Argument> & arguments) {
    Signature signature;
    signature.name = name;
    signature.body = body;
    signature.result = result;
    signature.arguments = arguments;
    return signature;
}

/**
 * A function whose result is the type of one of its arguments: normalize() of a normal is a
 * normal, and mix() of two colours is a colour.
 **/
Signature same(const char* name, Body body, int argument, const std::vector<Argument> & arguments) {
    Signature signature;
    signature.name = name;
    signature.body = body;
    signature.resultFrom = argument;
    signature.arguments = arguments;
    return signature;
}

/**
 * A function that returns its results through the arguments from `first` on rather than
 * through a return value.
 **/
Signature writing(const char* name, Body body, int first, const std::vector<Argument> & arguments) {
    Signature signature = declare(name, body, Type::VOID, arguments);
    signature.outputs = first;
    return signature;
}

/**
 * A function that changes its first argument in place and returns nothing.
 **/
Signature updating(const char* name, Body body, const std::vector<Argument> & arguments) {
    Signature signature = declare(name, body, Type::VOID, arguments);
    signature.updates = 0;
    return signature;
}

Signature shading(const char* name, Body body, Type result, const std::vector<Argument> & arguments) {
    Signature signature = declare(name, body, result, arguments);
    signature.varying = true;
    return signature;
}

std::vector<Signature> build() {
    std::vector<Signature> table;

    // maths, which is arithmetic over the value model
    const struct { const char* name; Body body; } unary[] = {
        { "abs", Body::ABS }, { "sign", Body::SIGN }, { "floor", Body::FLOOR },
        { "ceil", Body::CEIL }, { "round", Body::ROUND }, { "sqrt", Body::SQRT },
        { "exp", Body::EXP }, { "log", Body::LOG }, { "radians", Body::RADIANS },
        { "degrees", Body::DEGREES }, { "sin", Body::SIN }, { "cos", Body::COS },
        { "tan", Body::TAN }, { "asin", Body::ASIN }, { "acos", Body::ACOS },
        { "atan", Body::ATAN }
    };
    for (const auto & entry : unary) {
        table.push_back(declare(entry.name, entry.body, Type::FLOAT, { Argument::FLOAT }));
    }
    table.push_back(declare("atan", Body::ATAN, Type::FLOAT, { Argument::FLOAT, Argument::FLOAT }));
    table.push_back(declare("log", Body::LOG, Type::FLOAT, { Argument::FLOAT, Argument::FLOAT }));
    table.push_back(declare("mod", Body::MOD, Type::FLOAT, { Argument::FLOAT, Argument::FLOAT }));
    table.push_back(declare("pow", Body::POW, Type::FLOAT, { Argument::FLOAT, Argument::FLOAT }));
    table.push_back(same("min", Body::MIN, 0, { Argument::NUMBER, Argument::NUMBER }));
    table.push_back(same("max", Body::MAX, 0, { Argument::NUMBER, Argument::NUMBER }));
    table.push_back(same("clamp", Body::CLAMP, 0, { Argument::NUMBER, Argument::NUMBER, Argument::NUMBER }));
    table.push_back(same("mix", Body::MIX, 0, { Argument::NUMBER, Argument::NUMBER, Argument::FLOAT }));
    table.push_back(declare("step", Body::STEP, Type::FLOAT, { Argument::FLOAT, Argument::FLOAT }));
    table.push_back(declare("smoothstep", Body::SMOOTHSTEP, Type::FLOAT,
        { Argument::FLOAT, Argument::FLOAT, Argument::FLOAT }));

    // geometry
    table.push_back(declare("length", Body::LENGTH, Type::FLOAT, { Argument::POINTLIKE }));
    table.push_back(declare("distance", Body::DISTANCE, Type::FLOAT, { Argument::POINT, Argument::POINT }));
    table.push_back(same("normalize", Body::NORMALIZE, 0, { Argument::POINTLIKE }));
    table.push_back(same("faceforward", Body::FACEFORWARD, 0, { Argument::POINTLIKE, Argument::POINTLIKE }));
    table.push_back(same("faceforward", Body::FACEFORWARD, 0,
        { Argument::POINTLIKE, Argument::POINTLIKE, Argument::POINTLIKE }));
    table.push_back(declare("reflect", Body::REFLECT, Type::VECTOR, { Argument::POINTLIKE, Argument::POINTLIKE }));
    table.push_back(declare("refract", Body::REFRACT, Type::VECTOR,
        { Argument::POINTLIKE, Argument::POINTLIKE, Argument::FLOAT }));
    // how much of a ray a dielectric reflects and how much it lets through, with refract's
    // conventions: eta is the ratio of the indices on the incident side and the far side
    table.push_back(writing("fresnel", Body::FRESNEL, 3, { Argument::POINTLIKE, Argument::POINTLIKE,
        Argument::FLOAT, Argument::FLOAT, Argument::FLOAT }));
    table.push_back(writing("fresnel", Body::FRESNEL, 3, { Argument::POINTLIKE, Argument::POINTLIKE,
        Argument::FLOAT, Argument::FLOAT, Argument::FLOAT, Argument::POINTLIKE, Argument::POINTLIKE }));
    table.push_back(declare("depth", Body::DEPTH, Type::FLOAT, { Argument::POINT }));
    table.push_back(shading("calculatenormal", Body::STUB, Type::NORMAL, { Argument::POINT }));

    // the component accessors and their setters
    const struct { const char* name; Body body; } readers[] = {
        { "xcomp", Body::XCOMP }, { "ycomp", Body::YCOMP }, { "zcomp", Body::ZCOMP }
    };
    for (const auto & entry : readers) {
        table.push_back(declare(entry.name, entry.body, Type::FLOAT, { Argument::POINTLIKE }));
    }
    const struct { const char* name; Body body; } writers[] = {
        { "setxcomp", Body::SETXCOMP }, { "setycomp", Body::SETYCOMP },
        { "setzcomp", Body::SETZCOMP }
    };
    for (const auto & entry : writers) {
        table.push_back(updating(entry.name, entry.body, { Argument::POINTLIKE, Argument::FLOAT }));
    }
    table.push_back(declare("comp", Body::COMP, Type::FLOAT, { Argument::NUMBER, Argument::FLOAT }));
    table.push_back(updating("setcomp", Body::SETCOMP, { Argument::NUMBER, Argument::FLOAT, Argument::FLOAT }));

    // the transforms, which treat the three point-like types differently
    table.push_back(declare("ptransform", Body::PTRANSFORM, Type::POINT, { Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("ptransform", Body::PTRANSFORM, Type::POINT,
        { Argument::STRING, Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("vtransform", Body::VTRANSFORM, Type::VECTOR, { Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("vtransform", Body::VTRANSFORM, Type::VECTOR,
        { Argument::STRING, Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("ntransform", Body::NTRANSFORM, Type::NORMAL, { Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("ntransform", Body::NTRANSFORM, Type::NORMAL,
        { Argument::STRING, Argument::STRING, Argument::POINTLIKE }));
    table.push_back(declare("ctransform", Body::CTRANSFORM, Type::COLOR, { Argument::STRING, Argument::COLOR }));
    table.push_back(declare("ctransform", Body::CTRANSFORM, Type::COLOR,
        { Argument::STRING, Argument::STRING, Argument::COLOR }));
    table.push_back(declare("mtransform", Body::MTRANSFORM, Type::MATRIX, { Argument::STRING, Argument::MATRIX }));

    // matrix
    table.push_back(declare("determinant", Body::DETERMINANT, Type::FLOAT, { Argument::MATRIX }));
    table.push_back(declare("translate", Body::TRANSLATE, Type::MATRIX, { Argument::MATRIX, Argument::POINTLIKE }));
    table.push_back(declare("rotate", Body::ROTATE, Type::MATRIX,
        { Argument::MATRIX, Argument::FLOAT, Argument::POINTLIKE }));
    table.push_back(declare("scale", Body::SCALE, Type::MATRIX, { Argument::MATRIX, Argument::POINTLIKE }));

    // the light model, written over illuminance rather than given privileged access
    table.push_back(shading("ambient", Body::AMBIENT, Type::COLOR, {}));
    table.push_back(shading("diffuse", Body::SOURCE, Type::COLOR, { Argument::POINTLIKE }));
    table.push_back(shading("specular", Body::SOURCE, Type::COLOR,
        { Argument::POINTLIKE, Argument::POINTLIKE, Argument::FLOAT }));
    table.push_back(shading("phong", Body::SOURCE, Type::COLOR,
        { Argument::POINTLIKE, Argument::POINTLIKE, Argument::FLOAT }));
    table.push_back(declare("specularbrdf", Body::SOURCE, Type::COLOR,
        { Argument::POINTLIKE, Argument::POINTLIKE, Argument::POINTLIKE, Argument::FLOAT }));

    // computed by the renderer rather than the machine: a shadow, and a traced ray
    table.push_back(shading("transmission", Body::TRANSMISSION, Type::COLOR, { Argument::POINT, Argument::POINT }));
    table.push_back(shading("trace", Body::TRACE, Type::COLOR, { Argument::POINT, Argument::POINTLIKE }));

    // an image, read at s and t when no coordinates are given. A colour unless a cast requests
    // a float, which is the first channel
    for (const Type result : { Type::COLOR, Type::FLOAT }) {
        table.push_back(shading("texture", Body::TEXTURE, result, { Argument::STRING }));
        table.push_back(shading("texture", Body::TEXTURE, result,
            { Argument::STRING, Argument::FLOAT, Argument::FLOAT }));
    }
    // a float unless a cast requests three of them, each its own pattern
    for (const Type result : { Type::FLOAT, Type::COLOR, Type::POINT, Type::VECTOR }) {
        table.push_back(declare("noise", Body::NOISE, result, { Argument::FLOAT }));
        table.push_back(declare("noise", Body::NOISE, result, { Argument::FLOAT, Argument::FLOAT }));
        table.push_back(declare("noise", Body::NOISE, result, { Argument::POINT }));
    }

    // declared, stubbed and reported once
    table.push_back(shading("shadow", Body::STUB, Type::FLOAT, { Argument::STRING, Argument::POINT }));

    // printf, for debugging
    Signature print = declare("printf", Body::PRINTF, Type::VOID, { Argument::STRING });
    print.variadic = true;
    table.push_back(print);

    return table;
}

/*
    The light model, in the language. L points from the point being shaded toward the
    light - the same direction in a light shader's illuminate and in a surface shader's
    illuminance body - so a cosine falloff is L . N and no term here negates anything.

    A cone of PI/2 covers the front of the surface, so a light behind it is left out of
    the sum.
*/
const char* const SOURCE = R"(
surface library() {
    color diffuse(normal Nn) {
        color C = 0;
        illuminance(P, Nn, 1.5707963) {
            C += Cl * (normalize(L) . Nn);
        }
        return C;
    }
    color specularbrdf(vector Ln; normal Nn; vector V; float roughness) {
        vector H = normalize(Ln + V);
        return color (pow(max(0, Nn . H), 1 / roughness));
    }
    color specular(normal Nn; vector V; float roughness) {
        color C = 0;
        illuminance(P, Nn, 1.5707963) {
            C += Cl * specularbrdf(normalize(L), Nn, V, roughness);
        }
        return C;
    }
    color phong(normal Nn; vector V; float size) {
        color C = 0;
        illuminance(P, Nn, 1.5707963) {
            vector R = reflect(-normalize(L), Nn);
            C += Cl * pow(max(0, R . V), size);
        }
        return C;
    }
}
)";

std::vector<syntax::Function> read() {
    std::istringstream stream(SOURCE);
    Parser parser(stream);
    const std::vector<syntax::ShaderPtr> shaders = parser.parse();
    // the wrapper is a shader only because a function is parsed inside one; nothing but its
    // function list is kept
    return shaders.size() == 1 ? shaders[0]->functions : std::vector<syntax::Function>();
}

};  // namespace

std::vector<syntax::Function> sources() {
    return read();
}

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

};  // namespace v3d::render::offline::sl

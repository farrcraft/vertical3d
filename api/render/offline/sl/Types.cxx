/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Types.h"

#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::offline::sl {

const char* name(Type type) {
    switch (type) {
        case Type::VOID: return "void";
        case Type::FLOAT: return "float";
        case Type::POINT: return "point";
        case Type::VECTOR: return "vector";
        case Type::NORMAL: return "normal";
        case Type::COLOR: return "color";
        case Type::MATRIX: return "matrix";
        case Type::STRING: return "string";
    }
    return "";
}

const char* name(ShaderType type) {
    switch (type) {
        case ShaderType::SURFACE: return "surface";
        case ShaderType::LIGHT: return "light";
        case ShaderType::DISPLACEMENT: return "displacement";
        case ShaderType::VOLUME: return "volume";
        case ShaderType::IMAGER: return "imager";
    }
    return "";
}

unsigned int components(Type type) {
    switch (type) {
        case Type::FLOAT:
            return 1;
        case Type::POINT:
        case Type::VECTOR:
        case Type::NORMAL:
        case Type::COLOR:
            return 3;
        case Type::MATRIX:
            return 16;
        case Type::STRING:
        case Type::VOID:
            return 0;
    }
    return 0;
}

bool pointlike(Type type) {
    return type == Type::POINT || type == Type::VECTOR || type == Type::NORMAL;
}

bool coercible(Type from, Type to) {
    if (from == to) {
        return true;
    }
    if (from == Type::VOID || to == Type::VOID || from == Type::STRING || to == Type::STRING) {
        return false;
    }
    // a float replicates into anything that is made of floats, which is what lets a shader
    // write "color specularcolor = 1"
    if (from == Type::FLOAT) {
        return true;
    }
    return pointlike(from) && pointlike(to);
}

Type arithmetic(Type left, Type right) {
    if (left == Type::VOID || right == Type::VOID ||
        left == Type::STRING || right == Type::STRING) {
        return Type::VOID;
    }
    if (left == right) {
        return left;
    }
    // a float is a scale over anything else, whichever side it is on
    if (left == Type::FLOAT) {
        return right;
    }
    if (right == Type::FLOAT) {
        return left;
    }
    // the three point-like types mix, and the result takes the left operand's - RI's own
    // affine reading, where a point minus a point is a vector, is a distinction this
    // language does not draw
    if (pointlike(left) && pointlike(right)) {
        return left;
    }
    // a colour and a position have no arithmetic between them, and ctransform is what a
    // shader that means to convert one says
    return Type::VOID;
}

glm::vec3 ptransform(const glm::mat4x4 & matrix, const glm::vec3 & point) {
    return glm::vec3(matrix * glm::vec4(point, 1.0f));
}

glm::vec3 vtransform(const glm::mat4x4 & matrix, const glm::vec3 & vector) {
    return glm::vec3(matrix * glm::vec4(vector, 0.0f));
}

glm::vec3 ntransform(const glm::mat4x4 & matrix, const glm::vec3 & normal) {
    return glm::transpose(glm::inverse(glm::mat3x3(matrix))) * normal;
}

};  // namespace v3d::render::offline::sl

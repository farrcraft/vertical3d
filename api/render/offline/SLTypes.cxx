/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SLTypes.h"

#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::offline {

unsigned int components(SLType type) {
    switch (type) {
        case SLType::FLOAT:
            return 1;
        case SLType::POINT:
        case SLType::VECTOR:
        case SLType::NORMAL:
        case SLType::COLOR:
            return 3;
        case SLType::MATRIX:
            return 16;
        case SLType::STRING:
        case SLType::VOID:
            return 0;
    }
    return 0;
}

bool pointlike(SLType type) {
    return type == SLType::POINT || type == SLType::VECTOR || type == SLType::NORMAL;
}

bool coercible(SLType from, SLType to) {
    if (from == to) {
        return true;
    }
    if (from == SLType::VOID || to == SLType::VOID || from == SLType::STRING || to == SLType::STRING) {
        return false;
    }
    // a float replicates into anything that is made of floats, which is what lets a shader
    // write "color specularcolor = 1"
    if (from == SLType::FLOAT) {
        return true;
    }
    return pointlike(from) && pointlike(to);
}

SLType arithmetic(SLType left, SLType right) {
    if (left == SLType::VOID || right == SLType::VOID ||
        left == SLType::STRING || right == SLType::STRING) {
        return SLType::VOID;
    }
    if (left == right) {
        return left;
    }
    // a float is a scale over anything else, whichever side it is on
    if (left == SLType::FLOAT) {
        return right;
    }
    if (right == SLType::FLOAT) {
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
    return SLType::VOID;
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

};  // namespace v3d::render::offline

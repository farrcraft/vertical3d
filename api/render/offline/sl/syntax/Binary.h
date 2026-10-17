/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

/**
 * Everything infix, including two operators that differ from C: '.' is a dot product
 * rather than a member access, and '^' is a cross product rather than an exponent.
 **/
class Binary final : public Expression {
 public:
    /** The two sides of the operator, in source order. **/
    struct Operands {
        ExpressionPtr left;
        ExpressionPtr right;
    };

    Binary(const std::string & op, const Operands & operands, const v3d::render::offline::Position & at);

    std::string op;
    ExpressionPtr left;
    ExpressionPtr right;
};

};  // namespace v3d::render::offline::sl::syntax

/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

/**
 * Everything infix, including the two that read as something else: '.' is a dot product
 * rather than a member access, and '^' is a cross product rather than an exponent.
 **/
class Binary final : public Expression {
 public:
    Binary(const std::string & op, const ExpressionPtr & left, const ExpressionPtr & right,
        unsigned int line, unsigned int column);

    std::string op;
    ExpressionPtr left;
    ExpressionPtr right;
};

};  // namespace v3d::render::offline::sl::syntax

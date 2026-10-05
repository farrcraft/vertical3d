/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

/**
 * Negation or logical not, the two prefix operators the language has.
 **/
class Unary final : public Expression {
 public:
    Unary(const std::string & op, const ExpressionPtr & operand, unsigned int line, unsigned int column);

    std::string op;
    ExpressionPtr operand;
};

};  // namespace v3d::render::offline::sl::syntax

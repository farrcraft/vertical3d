/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Expression.h"
#include "Statement.h"

namespace v3d::render::offline::sl::syntax {

/**
 * An assignment and its compound forms. The target is an expression rather than a name so
 * that `Ci[0] = 1` parses; whether it is something that can be assigned to is the
 * compiler's answer.
 **/
class Assignment final : public Statement {
 public:
    Assignment(const std::string & op, const ExpressionPtr & target, const ExpressionPtr & value,
        unsigned int line, unsigned int column);

    std::string op;
    ExpressionPtr target;
    ExpressionPtr value;
};

};  // namespace v3d::render::offline::sl::syntax

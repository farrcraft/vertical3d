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
 * that `Ci[0] = 1` parses; the compiler checks whether it can be assigned to.
 **/
class Assignment final : public Statement {
 public:
    /** What is assigned to, and what is assigned. **/
    struct Sides {
        ExpressionPtr target;
        ExpressionPtr value;
    };

    Assignment(const std::string & op, const Sides & sides, const v3d::render::offline::Position & at);

    std::string op;
    ExpressionPtr target;
    ExpressionPtr value;
};

};  // namespace v3d::render::offline::sl::syntax

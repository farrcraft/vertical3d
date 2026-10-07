/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Expression.h"
#include "Statement.h"

namespace v3d::render::offline::sl::syntax {

class For final : public Statement {
 public:
    For(unsigned int line, unsigned int column);

    /** Any of the three heads may be null, when it is empty in the source. **/
    StatementPtr initialiser;
    ExpressionPtr condition;
    StatementPtr step;
    StatementPtr body;
};

};  // namespace v3d::render::offline::sl::syntax

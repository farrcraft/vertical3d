/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Expression.h"
#include "Statement.h"

namespace v3d::render::offline::sl::syntax {

class Conditional final : public Statement {
 public:
    Conditional(const ExpressionPtr & condition, const StatementPtr & whenTrue,
        const StatementPtr & whenFalse, unsigned int line, unsigned int column);

    ExpressionPtr condition;
    StatementPtr whenTrue;
    /** Null when there was no else. **/
    StatementPtr whenFalse;
};

};  // namespace v3d::render::offline::sl::syntax

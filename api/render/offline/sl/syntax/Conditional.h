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
    /** What runs when the condition holds, and what runs when it does not, which may be null. **/
    struct Branches {
        StatementPtr whenTrue;
        StatementPtr whenFalse;
    };

    Conditional(const ExpressionPtr & condition, const Branches & branches, const v3d::render::offline::Position & at);

    ExpressionPtr condition;
    StatementPtr whenTrue;
    /** Null when there was no else. **/
    StatementPtr whenFalse;
};

};  // namespace v3d::render::offline::sl::syntax

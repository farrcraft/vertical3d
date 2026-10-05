/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Expression.h"
#include "Statement.h"

namespace v3d::render::offline::sl::syntax {

class While final : public Statement {
 public:
    While(const ExpressionPtr & condition, const StatementPtr & body, unsigned int line, unsigned int column);

    ExpressionPtr condition;
    StatementPtr body;
};

};  // namespace v3d::render::offline::sl::syntax

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
    While(const ExpressionPtr & condition, const StatementPtr & body, const v3d::render::offline::Position & at);

    ExpressionPtr condition;
    StatementPtr body;
};

};  // namespace v3d::render::offline::sl::syntax

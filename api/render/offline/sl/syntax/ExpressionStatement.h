/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Expression.h"
#include "Statement.h"

namespace v3d::render::offline::sl::syntax {

/**
 * An expression run as a statement for its effect, such as a call.
 **/
class ExpressionStatement final : public Statement {
 public:
    ExpressionStatement(const ExpressionPtr & expression, const v3d::render::offline::source::Position & at);

    ExpressionPtr expression;
};

};  // namespace v3d::render::offline::sl::syntax

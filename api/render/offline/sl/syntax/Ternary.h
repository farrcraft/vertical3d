/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

class Ternary final : public Expression {
 public:
    Ternary(const ExpressionPtr & condition, const ExpressionPtr & whenTrue,
        const ExpressionPtr & whenFalse, unsigned int line, unsigned int column);

    ExpressionPtr condition;
    ExpressionPtr whenTrue;
    ExpressionPtr whenFalse;
};

};  // namespace v3d::render::offline::sl::syntax

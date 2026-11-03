/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

/**
 * A conditional expression: a condition, the value when it holds, and the value when it does not.
 **/
class Ternary final : public Expression {
 public:
    /** The condition, and the values when it holds and when it does not. **/
    struct Branches {
        ExpressionPtr condition;
        ExpressionPtr whenTrue;
        ExpressionPtr whenFalse;
    };

    Ternary(const Branches & branches, const v3d::render::offline::source::Position & at);

    ExpressionPtr condition;
    ExpressionPtr whenTrue;
    ExpressionPtr whenFalse;
};

};  // namespace v3d::render::offline::sl::syntax

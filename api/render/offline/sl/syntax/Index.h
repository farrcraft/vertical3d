/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

class Index final : public Expression {
 public:
    /** The array, and the expression that picks an element of it. **/
    struct Parts {
        ExpressionPtr array;
        ExpressionPtr index;
    };

    Index(const Parts & parts, const v3d::render::offline::Position & at);

    ExpressionPtr array;
    ExpressionPtr index;
};

};  // namespace v3d::render::offline::sl::syntax

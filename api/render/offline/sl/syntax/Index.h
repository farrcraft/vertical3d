/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Expression.h"

namespace v3d::render::offline::sl::syntax {

class Index final : public Expression {
 public:
    Index(const ExpressionPtr & array, const ExpressionPtr & index, unsigned int line, unsigned int column);

    ExpressionPtr array;
    ExpressionPtr index;
};

};  // namespace v3d::render::offline::sl::syntax
